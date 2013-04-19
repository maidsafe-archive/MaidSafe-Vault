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

#include "maidsafe/vault/maid_account_holder/maid_account.h"

#include <string>

#include "boost/variant/apply_visitor.hpp"

#include "maidsafe/common/utils.h"

#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/demultiplexer.h"
#include "maidsafe/vault/pmid_account_holder/pmid_account.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

PmidTotals::PmidTotals() : serialised_pmid_registration(), pmid_record() {}

PmidTotals::PmidTotals(
    const nfs::PmidRegistration::serialised_type& serialised_pmid_registration_in,
    const PmidRecord& pmid_record_in)
        : serialised_pmid_registration(serialised_pmid_registration_in),
          pmid_record(pmid_record_in) {}

PmidTotals::PmidTotals(const PmidTotals& other)
    : serialised_pmid_registration(other.serialised_pmid_registration),
      pmid_record(other.pmid_record) {}

PmidTotals::PmidTotals(PmidTotals&& other)
    : serialised_pmid_registration(std::move(other.serialised_pmid_registration)),
      pmid_record(std::move(other.pmid_record)) {}

PmidTotals& PmidTotals::operator=(PmidTotals other) {
  using std::swap;
  swap(serialised_pmid_registration, other.serialised_pmid_registration);
  swap(pmid_record, other.pmid_record);
  return *this;
}



MaidAccount::State::State()
    : id(0),
      pmid_totals(),
      total_claimed_available_size_by_pmids(0),
      total_put_data(0) {}

MaidAccount::State::State(const State& other)
    : id(other.id),
      pmid_totals(other.pmid_totals),
      total_claimed_available_size_by_pmids(other.total_claimed_available_size_by_pmids),
      total_put_data(other.total_put_data) {}

MaidAccount::State::State(State&& other)
    : id(std::move(other.id)),
      pmid_totals(std::move(other.pmid_totals)),
      total_claimed_available_size_by_pmids(std::move(other.total_claimed_available_size_by_pmids)),
      total_put_data(std::move(other.total_put_data)) {}

MaidAccount::State& MaidAccount::State::operator=(State other) {
  using std::swap;
  swap(id, other.id);
  swap(pmid_totals, other.pmid_totals);
  swap(total_claimed_available_size_by_pmids, other.total_claimed_available_size_by_pmids);
  swap(total_put_data, other.total_put_data);
  return *this;
}



MaidAccount::MaidAccount(const MaidName& maid_name, Db& db, const NodeId& this_node_id)
    : maid_name_(maid_name),
      state_(),
      account_db_(new AccountDb(db)),
      sync_(account_db_.get(), this_node_id) {}

MaidAccount::MaidAccount(const MaidName& maid_name,
                         const State& state,
                         Db& db,
                         const NodeId& this_node_id)
    : maid_name_(maid_name),
      state_(state),
      account_db_(new AccountDb(db)),
      sync_(account_db_.get(), this_node_id) {
  iterate through
  sync_.AddUnresolvedEntry()
  archive_.ApplyAccountTransfer(transferred_files_dir);
}

MaidAccount::MaidAccount(MaidAccount&& other)
    : maid_name_(std::move(other.maid_name_)),
      state_(std::move(other.state_)),
      account_db_(std::move(other.account_db_)),
      sync_(std::move(other.sync_)) {}

MaidAccount& MaidAccount::operator=(MaidAccount&& other) {
  maid_name_ = std::move(other.maid_name_);
  state_ = std::move(other.state_);
  account_db_= std::move(other.account_db_);
  sync_ = std::move(other.sync_);
  return *this;
}

void MaidAccount::ArchiveToDisk() const {
  protobuf::MaidAccount proto_maid_account;
  proto_maid_account.set_maid_name(maid_name_->string());

  proto_maid_account.set_state_id_number(confirmed_state_.id);
  for (auto& pmid_total : confirmed_state_.pmid_totals) {
    auto proto_pmid_totals(proto_maid_account.add_pmid_totals());
    proto_pmid_totals->set_serialised_pmid_registration(
        pmid_total.serialised_pmid_registration->string());
    *(proto_pmid_totals->mutable_pmid_record()) = pmid_total.pmid_record.ToProtobuf();
  }

  proto_maid_account.set_total_claimed_available_size_by_pmids(
      confirmed_state_.total_claimed_available_size_by_pmids);
  proto_maid_account.set_total_put_data(confirmed_state_.total_put_data);

  if (!WriteFile(archive_.GetAccountDir() / AccountFilename(),
                 proto_maid_account.SerializeAsString()))
    ThrowError(CommonErrors::filesystem_io_error);
}


std::vector<PmidTotals>::iterator MaidAccount::Find(const PmidName& pmid_name) {
  return std::find_if(std::begin(current_state_.pmid_totals),
                      std::end(current_state_.pmid_totals),
                      [&pmid_name](const PmidTotals& pmid_totals) {
                        return pmid_name == pmid_totals.pmid_record.pmid_name;
                      });
}

void MaidAccount::RegisterPmid(const nfs::PmidRegistration& pmid_registration) {
  auto itr(Find(pmid_registration.pmid_name()));
  if (itr == std::end(current_state_.pmid_totals)) {
    nfs::PmidRegistration::serialised_type serialised_pmid_registration(
        pmid_registration.Serialise());
    current_state_.pmid_totals.emplace_back(serialised_pmid_registration,
                                            PmidRecord(pmid_registration.pmid_name()));
  }
}

void MaidAccount::UnregisterPmid(const PmidName& pmid_name) {
  auto itr(Find(pmid_name));
  if (itr != current_state_.pmid_totals.end())
    current_state_.pmid_totals.erase(itr);
}

void MaidAccount::UpdatePmidTotals(const PmidTotals& pmid_totals) {
  auto itr(Find(pmid_totals.pmid_record.pmid_name));
  if (itr == current_state_.pmid_totals.end())
    ThrowError(CommonErrors::no_such_element);
  *itr = pmid_totals;
}

void MaidAccount::ApplySyncInfo(const State& state) {
}

void MaidAccount::ApplyTransferredFiles(const boost::filesystem::path& transferred_files_dir) {
}

std::vector<fs::path> MaidAccount::GetArchiveFileNames() const {
  return archive_.GetFilenames();
}

NonEmptyString MaidAccount::GetArchiveFile(const fs::path& filename) const {
  return archive_.GetFile(filename);
}

std::vector<DiskBasedStorage::RecentOperation> MaidAccount::GetRecentOps() const {
}

void MaidAccount::ReinstateUnmergedRecentOps(
      const std::vector<DiskBasedStorage::RecentOperation>& unmerged_recent_ops) {
}

void ApplyRecentOps(const std::vector<DiskBasedStorage::RecentOperation>& confirmed_recent_ops) {
}

boost::filesystem::path MaidAccount::GetAccountDir(const MaidName& maid_name,
                                                   const boost::filesystem::path& root) {
  return root / EncodeToBase32(maid_name_.data);
}

}  // namespace vault

}  // namespace maidsafe
