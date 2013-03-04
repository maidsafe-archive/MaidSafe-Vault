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

PmidRecord ParsePmidRecord(const PmidAccount::serialised_type& serialised_pmid_account) {
  protobuf::PmidAccount pmid_account;
  if (!pmid_account.ParseFromString(serialised_pmid_account.data.string()))
    ThrowError(CommonErrors::parsing_error);
  return PmidRecord(pmid_account.pmid_record());
}

}  // namespace

PmidAccount::DataElement::DataElement()
    : data_name_variant(),
      size() {}

PmidAccount::DataElement::DataElement(const DataNameVariant& data_name_variant_in,
                                      int32_t size_in)
    : data_name_variant(data_name_variant_in),
      size(size_in) {}

PmidAccount::DataElement::DataElement(const PmidAccount::DataElement& other)
    : data_name_variant(other.data_name_variant),
      size(other.size) {}

PmidAccount::DataElement& PmidAccount::DataElement::operator=(
    const PmidAccount::DataElement& other) {
  data_name_variant = other.data_name_variant;
  size = other.size;
  return *this;
}

PmidAccount::DataElement::DataElement(PmidAccount::DataElement&& other)
    : data_name_variant(std::move(other.data_name_variant)),
      size(std::move(other.size)) {}

PmidAccount::DataElement& PmidAccount::DataElement::operator=(PmidAccount::DataElement&& other) {
  data_name_variant = std::move(other.data_name_variant);
  size = std::move(other.size);
  return *this;
}

protobuf::DataElement PmidAccount::DataElement::ToProtobuf() const {
  GetTagValueAndIdentityVisitor type_and_name_visitor;
  auto type_and_name(boost::apply_visitor(type_and_name_visitor, data_name_variant));
  protobuf::DataElement data_element;
  data_element.set_name(type_and_name.second.string());
  data_element.set_type(static_cast<int32_t>(type_and_name.first));
  data_element.set_size(size);
  return data_element;
}

PmidAccount::PmidAccount(const PmidName& pmid_name, const boost::filesystem::path& root)
    : pmid_record_(pmid_name),
      data_holder_status_(DataHolderStatus::kGoingUp),
      recent_data_stored_(),
      kRoot_(root / EncodeToBase32(pmid_name->string())),
      archive_(kRoot_) {}

PmidAccount::PmidAccount(const serialised_type& serialised_pmid_account,
                         const boost::filesystem::path& root)
    : pmid_record_(ParsePmidRecord(serialised_pmid_account)),
      data_holder_status_(DataHolderStatus::kGoingUp),
      recent_data_stored_(),
      kRoot_(root / EncodeToBase32(pmid_record_.pmid_name->string())),
      archive_(kRoot_) {
  protobuf::PmidAccount pmid_account;
  if (!pmid_account.ParseFromString(serialised_pmid_account->string())) {
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

std::vector<PmidAccount::DataElement> PmidAccount::ParseArchiveFile(int32_t /*index*/) const {
//  std::vector<boost::filesystem::path> files(archive_.GetFileNames().get());
  std::vector<PmidAccount::DataElement> data_elements;
//  for (auto& file : files) {
//    if (static_cast<size_t>(index) == ExtractFileIndexFromFilename(file.filename().string())) {
//      protobuf::DiskStoredFile archived_pmid_data;
//      archived_pmid_data.ParseFromString(archive_.GetFile(file).get().string());
//      for (auto& data_record : archived_pmid_data.data_stored()) {
//        DataElement data_element(GetDataNameVariant(static_cast<DataTagValue>(data_record.type()),
//                                                    Identity(data_record.name())),
//                                 data_record.size());
//        data_elements.push_back(data_element);
//      }
//    }
//  }
  return data_elements;
}

void PmidAccount::ArchiveAccount() {
  try {
    ArchiveRecentData();
    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(pmid_record_.pmid_name->string()));
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
    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(pmid_record_.pmid_name->string()));
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
  std::vector<std::future<void>> archiving;
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

std::future<void> PmidAccount::ArchiveDataRecord(const PmidAccount::DataElement record) {
  std::future<void> result;
  GetTagValueAndIdentityVisitor type_and_name_visitor;
  auto type_and_name(boost::apply_visitor(type_and_name_visitor, record.data_name_variant));
  switch (type_and_name.first) {
    case DataTagValue::kAnmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kAnsmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kAntmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kAnmaidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kMaidValue: {
      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kPmidValue: {
      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kMidValue: {
      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kSmidValue: {
      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kTmidValue: {
      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kAnmpidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kMpidValue: {
      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kImmutableDataValue: {
      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kOwnerDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kGroupDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    case DataTagValue::kWorldDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
      result = archive_.Store<data_type>(data_type::name_type(type_and_name.second), record.size);
      break;
    }
    default:
      LOG(kError) << "Unhandled data type";
  }
  return std::move(result);
}

}  // namespace vault

}  // namespace maidsafe
