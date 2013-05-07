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

#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/pmid_account_holder/pmid_account.h"
#include "maidsafe/vault/unresolved_element.pb.h"


namespace maidsafe {
namespace vault {

const size_t PmidAccount::kSyncTriggerCount_(1);

//namespace {
//
//PmidRecord ParsePmidRecord(const PmidAccount::serialised_type& serialised_pmid_account) {
//  protobuf::PmidAccount pmid_account;
//  if (!pmid_account.ParseFromString(serialised_pmid_account.data.string()))
//    ThrowError(CommonErrors::parsing_error);
//  return PmidRecord(pmid_account.pmid_record());
//}
//
//}  // namespace

protobuf::DataElement PmidAccount::DataElement::ToProtobuf() const {
  GetTagValueAndIdentityVisitor type_and_name_visitor;
  auto type_and_name(boost::apply_visitor(type_and_name_visitor, data_name_variant));
  protobuf::DataElement data_element;
  data_element.set_name(type_and_name.second.string());
  data_element.set_type(static_cast<int32_t>(type_and_name.first));
  data_element.set_size(size);
  return data_element;
}

PmidAccount::PmidAccount(const PmidName& pmid_name,  Db& db, const NodeId& this_node_id)
    : pmid_name_(pmid_name),
      pmid_record_(pmid_name),
      data_holder_status_(DataHolderStatus::kUp),
      account_db_(new AccountDb(db)),
      sync_(account_db_.get(), this_node_id),
      account_transfer_nodes_(0) {}

PmidAccount::PmidAccount(const PmidName& pmid_name,
                         Db& db,
                         const NodeId& this_node_id,
                         const NodeId& source_id,
                         const serialised_type& serialised_pmid_account_details)
    : pmid_name_(pmid_name),
      pmid_record_(),
      account_db_(new AccountDb(db)),
      sync_(account_db_.get(), this_node_id),
      account_transfer_nodes_(routing::Parameters::node_group_size - 1) {
  ApplyAccountTransfer(source_id, serialised_pmid_account_details);

  //protobuf::PmidAccount pmid_account;
  //if (!pmid_account.ParseFromString(serialised_pmid_account_details->string())) {
  //  LOG(kError) << "Failed to parse pmid_account.";
  //  ThrowError(CommonErrors::parsing_error);
  //}
  ////for (auto& recent_data : pmid_account.recent_data_stored()) {
  ////  DataElement data_element(GetDataNameVariant(static_cast<DataTagValue>(recent_data.type()),
  ////                                              Identity(recent_data.name())),
  ////                           recent_data.size());
  ////  recent_data_stored_.push_back(data_element);
  ////}
}

PmidAccount::PmidAccount(PmidAccount&& other)
    : pmid_name_(std::move(other.pmid_name_)),
      pmid_record_(std::move(other.pmid_record_)),
      account_db_(std::move(other.account_db_)),
      sync_(std::move(other.sync_)),
      account_transfer_nodes_(std::move(other.account_transfer_nodes_)) {}

PmidAccount& PmidAccount::operator=(PmidAccount&& other) {
  pmid_name_ = std::move(other.pmid_name_);
  pmid_record_ = std::move(other.pmid_record_);
  account_db_ = std::move(other.account_db_);
  sync_ = std::move(other.sync_);
  account_transfer_nodes_ = std::move(other.account_transfer_nodes_);
  return *this;
}

PmidAccount::serialised_type PmidAccount::Serialise() {
  protobuf::PmidAccount pmid_account;
  *(pmid_account.mutable_pmid_record()) = pmid_record_.ToProtobuf();
  auto unresolved_data(sync_.GetUnresolvedData());
  for (const auto& unresolved_entry : unresolved_data)
    pmid_account.add_serialised_unresolved_entry(unresolved_entry.Serialise()->string());
  return serialised_type(NonEmptyString(pmid_account.SerializeAsString()));
}

bool PmidAccount::ApplyAccountTransfer(const NodeId& source_id,
                                       const serialised_type& serialised_pmid_account_details) {
  assert(account_transfer_nodes_);
  if (account_transfer_nodes_ == 0)
    return false;
  bool all_account_transfers_received(--account_transfer_nodes_ == 0);

  protobuf::PmidAccountDetails proto_pmid_account_details;
  if (!proto_pmid_account_details.ParseFromString(serialised_pmid_account_details.data.string()))
    ThrowError(CommonErrors::parsing_error);

  protobuf::PmidAccount proto_pmid_account;
  if (!proto_pmid_account.ParseFromString(proto_pmid_account_details.serialised_pmid_account()))
    ThrowError(CommonErrors::parsing_error);
  pmid_record_ = PmidRecord(proto_pmid_account.pmid_record());

  for (int i(0); i != proto_pmid_account_details.db_entry_size(); ++i) {
    auto data_name(GetDataNameVariant(
        static_cast<DataTagValue>(proto_pmid_account_details.db_entry(i).type()),
        Identity(proto_pmid_account_details.db_entry(i).name())));
    int32_t size(proto_pmid_account_details.db_entry(i).value().size());
    PmidAccountUnresolvedEntry entry(
        std::make_pair(data_name, nfs::MessageAction::kPut), size, source_id);
    if (sync_.AddAccountTransferRecord(entry, all_account_transfers_received)) {
      pmid_record_.stored_total_size += size;
    }
  }

  for (int i(0); i != proto_pmid_account_details.serialised_unresolved_entry_size(); ++i) {
    MaidAccountUnresolvedEntry entry(MaidAccountUnresolvedEntry::serialised_type(
        NonEmptyString(proto_pmid_account_details.serialised_unresolved_entry(i))));
    if (sync_.AddUnresolvedEntry(entry) && entry.messages_contents.front().value)
      pmid_record_.stored_total_size += *entry.messages_contents.front().value;
  }

  return all_account_transfers_received;
}

NonEmptyString PmidAccount::GetSyncData() {
  if (sync_.GetUnresolvedCount() < kSyncTriggerCount_)
    return NonEmptyString();

  auto unresolved_entries(sync_.GetUnresolvedData());
  if (unresolved_entries.empty())
    return NonEmptyString();

  protobuf::UnresolvedEntries proto_unresolved_entries;
  for (const auto& unresolved_entry : unresolved_entries) {
    proto_unresolved_entries.add_serialised_unresolved_entry(
        unresolved_entry.Serialise()->string());
  }
  return NonEmptyString(proto_unresolved_entries.SerializeAsString());
}

void PmidAccount::ApplySyncData(const NonEmptyString& serialised_unresolved_entries) {
  protobuf::UnresolvedEntries proto_unresolved_entries;
  if (!proto_unresolved_entries.ParseFromString(serialised_unresolved_entries.string()))
    ThrowError(CommonErrors::parsing_error);

  for (int i(0); i != proto_unresolved_entries.serialised_unresolved_entry_size(); ++i) {
    PmidAccountUnresolvedEntry entry(PmidAccountUnresolvedEntry::serialised_type(
        NonEmptyString(proto_unresolved_entries.serialised_unresolved_entry(i))));
    if (sync_.AddUnresolvedEntry(entry) && entry.messages_contents.front().value) {
      if (entry.key.second == nfs::MessageAction::kPut)
        pmid_record_.stored_total_size += *entry.messages_contents.front().value;
      else
        pmid_record_.stored_total_size -= *entry.messages_contents.front().value;
    }
  }
}

void PmidAccount::IncrementSyncAttempts() {
  sync_.IncrementSyncAttempts();
}

}  // namespace vault
}  // namespace maidsafe
