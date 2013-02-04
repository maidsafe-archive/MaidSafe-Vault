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

#include "maidsafe/vault/pmid_account.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

PmidAccount::DataElement::DataElement()
  : data_name_variant(), size() , type_and_name_visitor() {}

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

std::pair<DataTagValue, NonEmptyString> PmidAccount::DataElement::GetTypeAndName() const {
  auto type_and_name(boost::apply_visitor(type_and_name_visitor, data_name_variant));
  return type_and_name;
}

PmidAccount::PmidAccount(const PmidName& pmid_name, const boost::filesystem::path& root)
  : pmid_record_(pmid_name),
    data_holder_status_(DataHolderStatus::kGoingUp),
    recent_data_stored_(),
    archive_(root) {}

PmidAccount::PmidAccount(const serialised_type& serialised_pmid_account,
                         const boost::filesystem::path& root)
  : pmid_record_(),
    data_holder_status_(DataHolderStatus::kGoingUp),
    recent_data_stored_(),
    archive_(root) {
  protobuf::PmidAccount pmid_account;
  if (!pmid_account.ParseFromString(serialised_pmid_account.data.string())) {
    LOG(kError) << "Failed to parse pmid_account.";
    ThrowError(CommonErrors::parsing_error);
  }
  pmid_record_ = PmidRecord(pmid_account.pmid_record());
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

PmidAccount::serialised_type PmidAccount::Serialise() const {
  protobuf::PmidAccount pmid_account;
  *(pmid_account.mutable_pmid_record()) = pmid_record_.ToProtobuf();
  for (auto& record : recent_data_stored_)
    pmid_account.add_recent_data_stored()->CopyFrom(record.ToProtobuf());
  return serialised_type(NonEmptyString(pmid_account.SerializeAsString()));
}

std::vector< boost::filesystem::path > PmidAccount::GetArchiveFileNames() const {
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
}

std::future<void> PmidAccount::ArchiveDataRecord(const PmidAccount::DataElement record) {
  protobuf::DataElement data_element(record.ToProtobuf());
  std::future<void> archiving;
  auto type_and_name(record.GetTypeAndName());
  switch (type_and_name.first) {
    case DataTagValue::kAnmidValue:
      archiving = archive_.Store<passport::PublicAnmid>(
          passport::PublicAnmid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kAnsmidValue:
      archiving = archive_.Store<passport::PublicAnsmid>(
          passport::PublicAnsmid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kAntmidValue:
      archiving = archive_.Store<passport::PublicAntmid>(
          passport::PublicAntmid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kAnmaidValue:
      archiving = archive_.Store<passport::PublicAnmaid>(
          passport::PublicAnmaid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kMaidValue:
      archiving = archive_.Store<passport::PublicMaid>(
          passport::PublicMaid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kPmidValue:
      archiving = archive_.Store<passport::PublicPmid>(
          passport::PublicPmid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kMidValue:
      archiving = archive_.Store<passport::Mid>(
          passport::Mid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kSmidValue:
      archiving = archive_.Store<passport::Smid>(
          passport::Smid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kTmidValue:
      archiving = archive_.Store<passport::Tmid>(
          passport::Tmid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kAnmpidValue:
      archiving = archive_.Store<passport::PublicAnmpid>(
          passport::PublicAnmpid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kMpidValue:
      archiving = archive_.Store<passport::PublicMpid>(
          passport::PublicMpid::name_type(Identity(type_and_name.second.string())),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kImmutableDataValue:
      archiving = archive_.Store<ImmutableData>(ImmutableData::name_type(
                                                    Identity(type_and_name.second.string())),
                                                data_element.SerializeAsString());
      break;
    case DataTagValue::kMutableDataValue:
      archiving = archive_.Store<MutableData>(MutableData::name_type(
                                                  Identity(type_and_name.second.string())),
                                              data_element.SerializeAsString());
      break;
    default: {
      LOG(kError) << "Non handleable data type";
      ThrowError(CommonErrors::invalid_parameter);
    }
  }
  return std::move(archiving);
}

}  // namespace vault

}  // namespace maidsafe
