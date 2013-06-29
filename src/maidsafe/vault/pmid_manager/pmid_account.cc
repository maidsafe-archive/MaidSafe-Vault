/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include <string>

#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/pmid_manager/pmid_account.h"
#include "maidsafe/vault/unresolved_element.pb.h"


namespace maidsafe {
namespace vault {

const size_t PmidAccount::kSyncTriggerCount_(1);

PmidAccount::PmidAccount(const PmidName& pmid_name,  Db& db, const NodeId& this_node_id)
    : pmid_name_(pmid_name),
      pmid_record_(pmid_name),
      pmid_node_status_(DataHolderStatus::kUp),
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
  protobuf::PmidRecord proto_pmid_record;
  proto_pmid_record.set_pmid_name(pmid_record_.pmid_name.data.string());
  proto_pmid_record.set_stored_count(pmid_record_.stored_count);
  proto_pmid_record.set_stored_total_size(pmid_record_.stored_total_size);
  proto_pmid_record.set_lost_count(pmid_record_.lost_count);
  proto_pmid_record.set_lost_total_size(pmid_record_.lost_total_size);
  proto_pmid_record.set_claimed_available_size(pmid_record_.claimed_available_size);

  protobuf::PmidAccountDetails proto_pmid_account_details;

  proto_pmid_account_details.set_serialised_pmid_record(proto_pmid_record.SerializeAsString());

  auto db_entries(account_db_->Get());
  GetTagValueAndIdentityVisitor type_and_name_visitor;
  for (const auto& db_entry : db_entries) {
    auto name(db_entry.first.name());
    auto type_and_name(boost::apply_visitor(type_and_name_visitor, name));
    auto proto_db_entry(proto_pmid_account_details.add_db_entry());
    proto_db_entry->set_type(static_cast<uint32_t>(type_and_name.first));
    proto_db_entry->set_name(type_and_name.second.string());
    protobuf::PmidAccountDbValue proto_db_value;
    if (!proto_db_value.ParseFromString(db_entry.second.string()))
      ThrowError(CommonErrors::parsing_error);
    proto_db_entry->mutable_value()->CopyFrom(proto_db_value);
  }

  auto unresolved_data(sync_.GetUnresolvedData());
  for (const auto& unresolved_entry : unresolved_data) {
    proto_pmid_account_details.add_serialised_unresolved_entry(
        unresolved_entry.Serialise()->string());
  }

  return serialised_type(NonEmptyString(proto_pmid_account_details.SerializeAsString()));
}

void PmidAccount::PutData(int32_t size) {
  pmid_record_.stored_total_size += size;
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

  protobuf::PmidRecord proto_pmid_record;
  if (!proto_pmid_record.ParseFromString(proto_pmid_account_details.serialised_pmid_record()))
    ThrowError(CommonErrors::parsing_error);
  pmid_record_.pmid_name.data = Identity(proto_pmid_record.pmid_name());
  pmid_record_.stored_count = proto_pmid_record.stored_count();
  pmid_record_.stored_total_size = proto_pmid_record.stored_total_size();
  pmid_record_.lost_count = proto_pmid_record.lost_count();
  pmid_record_.lost_total_size = proto_pmid_record.lost_total_size();
  pmid_record_.claimed_available_size = proto_pmid_record.claimed_available_size();

  for (int i(0); i != proto_pmid_account_details.db_entry_size(); ++i) {
    auto data_name(GetDataNameVariant(
        static_cast<DataTagValue>(proto_pmid_account_details.db_entry(i).type()),
        Identity(proto_pmid_account_details.db_entry(i).name())));
    int32_t size(proto_pmid_account_details.db_entry(i).value().size());
    PmidAccountUnresolvedEntry entry(
        std::make_pair(data_name, nfs::MessageAction::kPut), size, source_id);
    if (sync_.AddAccountTransferRecord(entry, all_account_transfers_received).size() == 1U) {
      pmid_record_.stored_total_size += size;
    }
  }

  for (int i(0); i != proto_pmid_account_details.serialised_unresolved_entry_size(); ++i) {
    PmidAccountUnresolvedEntry entry(PmidAccountUnresolvedEntry::serialised_type(
        NonEmptyString(proto_pmid_account_details.serialised_unresolved_entry(i))));
    if (!sync_.AddUnresolvedEntry(entry).empty() && entry.messages_contents.front().value)
      pmid_record_.stored_total_size += *entry.messages_contents.front().value;
  }

  return all_account_transfers_received;
}

void PmidAccount::AddLocalUnresolvedEntry(const PmidAccountUnresolvedEntry& unresolved_entry) {
  sync_.AddLocalEntry(unresolved_entry);
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

std::vector<PmidAccountResolvedEntry> PmidAccount::ApplySyncData(const NonEmptyString& serialised_unresolved_entries) {
  std::vector<PmidAccountResolvedEntry> resolved_entries;
  protobuf::UnresolvedEntries proto_unresolved_entries;
  if (!proto_unresolved_entries.ParseFromString(serialised_unresolved_entries.string()))
    ThrowError(CommonErrors::parsing_error);

  for (int i(0); i != proto_unresolved_entries.serialised_unresolved_entry_size(); ++i) {
    PmidAccountUnresolvedEntry entry(PmidAccountUnresolvedEntry::serialised_type(
        NonEmptyString(proto_unresolved_entries.serialised_unresolved_entry(i))));
    resolved_entries = sync_.AddUnresolvedEntry(entry);
    if (!resolved_entries.empty() && entry.messages_contents.front().value) {
      if (entry.key.second == nfs::MessageAction::kPut)
        pmid_record_.stored_total_size += *entry.messages_contents.front().value;
      else
        pmid_record_.stored_total_size -= *entry.messages_contents.front().value;
    }
  }
  return resolved_entries;
}

void PmidAccount::ReplaceNodeInSyncList(const NodeId& old_node, const NodeId& new_node) {
  if (account_transfer_nodes_ != 0)
    --account_transfer_nodes_;
  sync_.ReplaceNode(old_node, new_node);
}

void PmidAccount::IncrementSyncAttempts() {
  sync_.IncrementSyncAttempts();
}

PmidRecord PmidAccount::GetPmidRecord() {
  return pmid_record_;
}

}  // namespace vault
}  // namespace maidsafe
