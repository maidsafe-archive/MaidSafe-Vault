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
#include "maidsafe/vault/pmid_account.pb.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

PmidAccount::PmidAccount(const PmidName& pmid_name, const boost::filesystem::path& root)
  : account_status_(Status::kNodeGoingUp),
    pmid_record_(pmid_name),
    recent_data_stored_(),
    type_and_name_visitor_(),
    archive_(root) {}

PmidAccount::PmidAccount(const serialised_type& serialised_pmid_account,
                         const boost::filesystem::path& root)
  : account_status_(Status::kNodeGoingUp),
    pmid_record_(),
    recent_data_stored_(),
    type_and_name_visitor_(),
    archive_(root) {
  protobuf::PmidAccount pmid_account;
  if (!pmid_account.ParseFromString(serialised_pmid_account.data.string())) {
    LOG(kError) << "Failed to parse pmid_account.";
    ThrowError(CommonErrors::parsing_error);
  }
  pmid_record_ = PmidRecord(pmid_account.pmid_record());
  for (auto& recent_data : pmid_account.recent_data_stored()) {
      recent_data_stored_.insert(std::make_pair(
          GetDataNameVariant(static_cast<DataTagValue>(recent_data.type()),
          Identity(recent_data.name())), recent_data.size()));
  }
}

PmidAccount::~PmidAccount() {
  ArchiveRecords();
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

void PmidAccount::ArchiveRecords() {
  std::vector<std::future<void>> archiving;
  for (auto& record : recent_data_stored_)
    archiving.emplace_back(ArchiveDataRecord(record.first, record.second));
  for (auto& archived : archiving)
    archived.get();
}

PmidAccount::serialised_type PmidAccount::Serialise() const {
  protobuf::PmidAccount pmid_account;
  *(pmid_account.mutable_pmid_record()) = pmid_record_.ToProtobuf();
  for (auto& record : recent_data_stored_) {
    auto type_and_name(boost::apply_visitor(type_and_name_visitor_, record.first));
    protobuf::DataElement data_element;
    data_element.set_name(type_and_name.second.string());
    data_element.set_type(static_cast<int32_t>(type_and_name.first));
    data_element.set_size(record.second);
    pmid_account.add_recent_data_stored()->CopyFrom(data_element);
  }
  return serialised_type(NonEmptyString(pmid_account.SerializeAsString()));
}

std::future<void> PmidAccount::ArchiveDataRecord(const DataNameVariant& data_name_variant,
                                                 const int32_t data_size) {
  auto type_and_name(boost::apply_visitor(type_and_name_visitor_, data_name_variant));
  protobuf::DataElement data_element;
  data_element.set_name(type_and_name.second.string());
  data_element.set_type(static_cast<int32_t>(type_and_name.first));
  data_element.set_size(data_size);
  std::future<void> archiving;

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
      archiving = archive_.Store<passport::Mid>(
          passport::Mid::name_type(type_and_name.second),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kSmidValue:
      archiving = archive_.Store<passport::Smid>(
          passport::Smid::name_type(type_and_name.second),
          data_element.SerializeAsString());
      break;
    case DataTagValue::kTmidValue:
      archiving = archive_.Store<passport::Tmid>(
          passport::Tmid::name_type(type_and_name.second),
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
    case DataTagValue::kMutableDataValue:
      archiving = archive_.Store<MutableData>(MutableData::name_type(type_and_name.second),
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
