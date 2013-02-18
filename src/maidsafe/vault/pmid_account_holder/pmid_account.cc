/***************************************************************************************************
 *  Copyright 2012 maidsafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use. The use of this code is governed by the licence file licence.txt found in the root of     *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit written *
 *  permission of the board of directors of MaidSafe.net.                                          *
 **************************************************************************************************/

#include <string>

#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

namespace {

size_t ExtractFileIndexFromFilename(const std::string& filename) {
  auto it(std::find(filename.begin(), filename.end(), '.'));
  if (it == filename.end())
    ThrowError(CommonErrors::unexpected_filename_format);
  return static_cast<size_t>(std::stoi(std::string(filename.begin(), it)));
}

PmidRecord ParsePmidRecord(const PmidAccount::serialised_type& serialised_pmid_account) {
  protobuf::PmidAccount pmid_account;
  if (!pmid_account.ParseFromString(serialised_pmid_account.data.string()))
    ThrowError(CommonErrors::parsing_error);
  return PmidRecord(pmid_account.pmid_record());
}

}  // namespace

typedef std::future<void> VoidFuture;

PmidAccount::DataElement::DataElement()
    : data_name_variant(),
      size() ,
      type_and_name_visitor() {}

PmidAccount::DataElement::DataElement(const DataNameVariant& data_name_variant_in,
                                      int32_t size_in)
  : data_name_variant(data_name_variant_in), size(size_in), type_and_name_visitor() {}

PmidAccount::DataElement::DataElement(const PmidAccount::DataElement& other)
  : data_name_variant(other.data_name_variant), size(other.size), type_and_name_visitor() {}

PmidAccount::DataElement& PmidAccount::DataElement::operator=(
    const PmidAccount::DataElement& other) {
  data_name_variant = other.data_name_variant;
  size = other.size;
  return *this;
}

PmidAccount::DataElement::DataElement(PmidAccount::DataElement&& other)
  : data_name_variant(std::move(other.data_name_variant)),
    size(std::move(other.size)),
    type_and_name_visitor() {}

PmidAccount::DataElement& PmidAccount::DataElement::operator=(PmidAccount::DataElement&& other) {
  data_name_variant = std::move(other.data_name_variant);
  size = std::move(other.size);
  return *this;
}

protobuf::DataElement PmidAccount::DataElement::ToProtobuf() const {
  auto type_and_name(boost::apply_visitor(type_and_name_visitor, data_name_variant));
  protobuf::DataElement data_element;
  data_element.set_name(type_and_name.second.string());
  data_element.set_type(static_cast<int32_t>(type_and_name.first));
  data_element.set_size(size);
  return data_element;
}

std::pair<DataTagValue, Identity> PmidAccount::DataElement::GetTypeAndName() const {
  std::pair<DataTagValue, Identity> type_and_name(boost::apply_visitor(type_and_name_visitor,
                                                                       data_name_variant));
  return type_and_name;
}

PmidAccount::PmidAccount(const PmidName& pmid_name, const boost::filesystem::path& root)
  : pmid_record_(pmid_name),
    data_holder_status_(DataHolderStatus::kGoingUp),
    recent_data_stored_(),
    kRoot_(root / EncodeToBase32(pmid_name.data.string())),
    archive_(kRoot_) {}

PmidAccount::PmidAccount(const serialised_type& serialised_pmid_account,
                         const boost::filesystem::path& root)
  : pmid_record_(ParsePmidRecord(serialised_pmid_account)),
    data_holder_status_(DataHolderStatus::kGoingUp),
    recent_data_stored_(),
    kRoot_(root / EncodeToBase32(pmid_record_.pmid_name.data.string())),
    archive_(kRoot_) {
  protobuf::PmidAccount pmid_account;
  if (!pmid_account.ParseFromString(serialised_pmid_account.data.string())) {
    LOG(kError) << "Failed to parse pmid_account.";
    ThrowError(CommonErrors::parsing_error);
  }
  for (auto& recent_data : pmid_account.recent_data_stored()) {
    DataElement data_element(GetDataNameVariant(static_cast<DataTagValue>(recent_data.type()),
                                                Identity(recent_data.name())),
                             recent_data.size());
    recent_data_stored_.push_back(data_element);
  }
}

PmidAccount::~PmidAccount() {
  ArchiveAccount();
}

void PmidAccount::SetDataHolderGoingUp() {
  RestoreRecentData();
  data_holder_status_ = DataHolderStatus::kGoingUp;
}

void PmidAccount::SetDataHolderGoingDown() {
  ArchiveRecentData();
  data_holder_status_ = DataHolderStatus::kGoingDown;
}

std::vector<PmidAccount::DataElement> PmidAccount::ParseArchiveFile(int32_t index) const {
  std::vector<boost::filesystem::path> files(archive_.GetFileNames().get());
  std::vector<PmidAccount::DataElement> data_elements;
  for (auto& file : files) {
    if (static_cast<size_t>(index) == ExtractFileIndexFromFilename(file.filename().string())) {
      protobuf::DiskStoredFile archived_pmid_data;
      archived_pmid_data.ParseFromString(archive_.GetFile(file).get().string());
      for (auto& data_record : archived_pmid_data.data_stored()) {
        DataElement data_element(GetDataNameVariant(static_cast<DataTagValue>(data_record.type()),
                                                    Identity(data_record.name())),
                                 data_record.size());
        data_elements.push_back(data_element);
      }
    }
  }
  return data_elements;
}

void PmidAccount::ArchiveAccount() {
  try {
    ArchiveRecentData();
    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(pmid_record_.pmid_name.data.string()));
    std::string file_name(EncodeToBase32(hash));
    maidsafe::WriteFile(kRoot_ / file_name, pmid_record_.ToProtobuf().SerializeAsString());
  }
  catch(const std::exception& e) {
    LOG(kError) << "error in archive memory data or account info: " << e.what();
    ThrowError(CommonErrors::filesystem_io_error);
  }
}

void PmidAccount::RestoreAccount() {
  try {
    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(pmid_record_.pmid_name.data.string()));
    std::string file_name(EncodeToBase32(hash));
    NonEmptyString content(maidsafe::ReadFile(kRoot_ / file_name));
    protobuf::PmidRecord pmid_record;
    pmid_record.ParseFromString(content.string());
    pmid_record_ = PmidRecord(pmid_record);
    RestoreRecentData();
  }
  catch(const std::exception& e) {
    LOG(kError) << "error in restore archived data and account info: " << e.what();
    ThrowError(CommonErrors::filesystem_io_error);
  }
}

PmidAccount::serialised_type PmidAccount::Serialise() const {
  protobuf::PmidAccount pmid_account;
  *(pmid_account.mutable_pmid_record()) = pmid_record_.ToProtobuf();
  for (auto& record : recent_data_stored_)
    pmid_account.add_recent_data_stored()->CopyFrom(record.ToProtobuf());
  return serialised_type(NonEmptyString(pmid_account.SerializeAsString()));
}

std::vector<boost::filesystem::path> PmidAccount::GetArchiveFileNames() const {
  return archive_.GetFileNames().get();
}

NonEmptyString PmidAccount::GetArchiveFile(const boost::filesystem::path& path) const {
  return archive_.GetFile(path).get();
}

void PmidAccount::PutArchiveFile(const boost::filesystem::path& path,
                                 const NonEmptyString& content) {
  archive_.PutFile(path, content);
}

void PmidAccount::ArchiveRecentData() {
  std::vector<VoidFuture> archiving;
  for (auto& record : recent_data_stored_)
    archiving.emplace_back(ArchiveDataRecord(record));
  for (auto& archived : archiving)
    archived.get();
  recent_data_stored_.clear();
}

void PmidAccount::RestoreRecentData() {
  int32_t file_count(archive_.GetFileCount().get());
  std::vector<DataElement> latest_elements(ParseArchiveFile(file_count - 1));
  for (auto& element : latest_elements)
    recent_data_stored_.push_back(element);
}

VoidFuture PmidAccount::ArchiveDataRecord(const PmidAccount::DataElement record) {
  protobuf::DataElement data_element(record.ToProtobuf());
  VoidFuture archiving;
  auto type_and_name(record.GetTypeAndName());
  switch (type_and_name.first) {
    case DataTagValue::kAnmidValue:
      archiving = archive_.Store<passport::PublicAnmid>(
                      passport::PublicAnmid::name_type(type_and_name.second),
                      data_element.SerializeAsString());
      break;
    case DataTagValue::kAnsmidValue:
      archiving = archive_.Store<passport::PublicAnsmid>(
                      passport::PublicAnsmid::name_type(type_and_name.second),
                      data_element.SerializeAsString());
      break;
    case DataTagValue::kAntmidValue:
      archiving = archive_.Store<passport::PublicAntmid>(
                      passport::PublicAntmid::name_type(type_and_name.second),
                      data_element.SerializeAsString());
      break;
    case DataTagValue::kAnmaidValue:
      archiving = archive_.Store<passport::PublicAnmaid>(
                      passport::PublicAnmaid::name_type(type_and_name.second),
                      data_element.SerializeAsString());
      break;
    case DataTagValue::kMaidValue:
      archiving = archive_.Store<passport::PublicMaid>(
                      passport::PublicMaid::name_type(type_and_name.second),
                      data_element.SerializeAsString());
      break;
    case DataTagValue::kPmidValue:
      archiving = archive_.Store<passport::PublicPmid>(
                      passport::PublicPmid::name_type(type_and_name.second),
                      data_element.SerializeAsString());
      break;
    case DataTagValue::kMidValue:
      archiving = archive_.Store<passport::Mid>(passport::Mid::name_type(type_and_name.second),
                                                data_element.SerializeAsString());
      break;
    case DataTagValue::kSmidValue:
      archiving = archive_.Store<passport::Smid>(passport::Smid::name_type(type_and_name.second),
                                                 data_element.SerializeAsString());
      break;
    case DataTagValue::kTmidValue:
      archiving = archive_.Store<passport::Tmid>(passport::Tmid::name_type(type_and_name.second),
                                                 data_element.SerializeAsString());
      break;
    case DataTagValue::kAnmpidValue:
      archiving = archive_.Store<passport::PublicAnmpid>(
                      passport::PublicAnmpid::name_type(type_and_name.second),
                      data_element.SerializeAsString());
      break;
    case DataTagValue::kMpidValue:
      archiving = archive_.Store<passport::PublicMpid>(
                      passport::PublicMpid::name_type(type_and_name.second),
                      data_element.SerializeAsString());
      break;
    case DataTagValue::kImmutableDataValue:
      archiving = archive_.Store<ImmutableData>(ImmutableData::name_type(type_and_name.second),
                                                data_element.SerializeAsString());
      break;
//    case DataTagValue::kMutableDataValue:  / TODO (Fraser) BEFORE_RELEASE  FIXME
//      archiving = archive_.Store<MutableData>(MutableData::name_type(type_and_name.second),
//                                              data_element.SerializeAsString());
//      break;
    default: {
      LOG(kError) << "Non handleable data type";
      ThrowError(CommonErrors::invalid_parameter);
    }
  }
  return std::move(archiving);
}

}  // namespace vault

}  // namespace maidsafe
