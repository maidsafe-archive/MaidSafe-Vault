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

PmidTotals& PmidTotals::operator=(const PmidTotals& other) {
  serialised_pmid_registration = other.serialised_pmid_registration;
  pmid_record = other.pmid_record;
  return *this;
}

PmidTotals::PmidTotals(PmidTotals&& other)
    : serialised_pmid_registration(std::move(other.serialised_pmid_registration)),
      pmid_record(std::move(other.pmid_record)) {}

PmidTotals& PmidTotals::operator=(PmidTotals&& other) {
  serialised_pmid_registration = std::move(other.serialised_pmid_registration);
  pmid_record = std::move(other.pmid_record);
  return *this;
}



MaidAccount::MaidAccount(const MaidName& maid_name, const fs::path& root)
    : maid_name_(maid_name),
      pmid_totals_(),
      total_claimed_available_size_by_pmids_(0),
      total_put_data_(0),
      recent_ops_(),
      archive_(root / EncodeToBase32(maid_name_.data)) {}

MaidAccount::MaidAccount(const serialised_type& serialised_maid_account, const fs::path& root)
    : maid_name_([&serialised_maid_account]()->Identity {
                     protobuf::MaidAccount proto_maid_account;
                     proto_maid_account.ParseFromString(serialised_maid_account->string());
                     return Identity(proto_maid_account.maid_name());
                 }()),
      pmid_totals_(),
      total_claimed_available_size_by_pmids_(0),
      total_put_data_(0),
      recent_ops_(),
      archive_(root / EncodeToBase32(maid_name_.data)) {
  protobuf::MaidAccount proto_maid_account;
  if (!proto_maid_account.ParseFromString(serialised_maid_account->string())) {
    LOG(kError) << "Failed to parse maid_account.";
    ThrowError(CommonErrors::parsing_error);
  }

  for (int i(0); i != proto_maid_account.pmid_totals_size(); ++i) {
    pmid_totals_.emplace_back(
        nfs::PmidRegistration::serialised_type(NonEmptyString(
            proto_maid_account.pmid_totals(i).serialised_pmid_registration())),
        PmidRecord(proto_maid_account.pmid_totals(i).pmid_record()));
  }

  total_claimed_available_size_by_pmids_ =
      proto_maid_account.total_claimed_available_size_by_pmids();
  total_put_data_ = proto_maid_account.total_put_data();
}

MaidAccount::MaidAccount(MaidAccount&& other)
    : maid_name_(std::move(other.maid_name_)),
      pmid_totals_(std::move(other.pmid_totals_)),
      total_claimed_available_size_by_pmids_(
          std::move(other.total_claimed_available_size_by_pmids_)),
      total_put_data_(std::move(other.total_put_data_)),
      recent_ops_(std::move(other.recent_ops_)),
      archive_(std::move(other.archive_)) {}

MaidAccount& MaidAccount::operator=(MaidAccount&& other) {
  maid_name_ = std::move(other.maid_name_);
  pmid_totals_ = std::move(other.pmid_totals_);
  total_claimed_available_size_by_pmids_ = std::move(other.total_claimed_available_size_by_pmids_);
  total_put_data_ = std::move(other.total_put_data_);
  recent_ops_ = std::move(other.recent_ops_);
  archive_ = std::move(other.archive_);
  return *this;
}

MaidAccount::serialised_type MaidAccount::Serialise() const {
  protobuf::MaidAccount proto_maid_account;
  proto_maid_account.set_maid_name(maid_name_->string());

  for (auto& pmid_total : pmid_totals_) {
    auto proto_pmid_totals(proto_maid_account.add_pmid_totals());
    proto_pmid_totals->set_serialised_pmid_registration(
        pmid_total.serialised_pmid_registration->string());
    *(proto_pmid_totals->mutable_pmid_record()) = pmid_total.pmid_record.ToProtobuf();
  }

  proto_maid_account.set_total_claimed_available_size_by_pmids(
      total_claimed_available_size_by_pmids_);
  proto_maid_account.set_total_put_data(total_put_data_);

  auto archive_file_names(GetArchiveFileNames());
  for (auto& archive_file_name : archive_file_names)
    proto_maid_account.add_archive_file_names(archive_file_name.string());

  return serialised_type(NonEmptyString(proto_maid_account.SerializeAsString()));
}

std::pair<MaidAccount::AccountInfo,
          std::vector<boost::filesystem::path>> MaidAccount::ParseAccountSyncInfo(
    const serialised_type& /*serialised_info*/) const {
  return std::make_pair(MaidAccount::AccountInfo(), std::vector<boost::filesystem::path>());
}

std::vector<PmidTotals>::iterator MaidAccount::Find(const PmidName& pmid_name) {
  return std::find_if(pmid_totals_.begin(),
                      pmid_totals_.end(),
                      [&pmid_name](const PmidTotals& pmid_totals) {
                        return pmid_name == pmid_totals.pmid_record.pmid_name;
                      });
}

void MaidAccount::RegisterPmid(
    const nfs::PmidRegistration& pmid_registration) {
  auto itr(Find(pmid_registration.pmid_name()));
  if (itr == pmid_totals_.end()) {
    nfs::PmidRegistration::serialised_type serialised_pmid_registration(
        pmid_registration.Serialise());
    pmid_totals_.emplace_back(serialised_pmid_registration,
                              PmidRecord(pmid_registration.pmid_name()));
  }
}

void MaidAccount::UnregisterPmid(const PmidName& pmid_name) {
  auto itr(Find(pmid_name));
  if (itr != pmid_totals_.end())
    pmid_totals_.erase(itr);
}

void MaidAccount::UpdatePmidTotals(const PmidTotals& pmid_totals) {
  auto itr(Find(pmid_totals.pmid_record.pmid_name));
  if (itr == pmid_totals_.end())
    ThrowError(CommonErrors::no_such_element);
  *itr = pmid_totals;
}

void MaidAccount::ApplyAccountTransfer(const fs::path& transferred_files_dir) {
  archive_.ApplyAccountTransfer(transferred_files_dir);
}

std::vector<fs::path> MaidAccount::GetArchiveFileNames() const {
  return archive_.GetFilenames();
}

NonEmptyString MaidAccount::GetArchiveFile(const fs::path& filename) const {
  return archive_.GetFile(filename);
}

}  // namespace vault

}  // namespace maidsafe
