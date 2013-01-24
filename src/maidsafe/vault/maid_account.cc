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

#include "maidsafe/vault/maid_account.h"

#include <string>

#include "maidsafe/common/utils.h"

#include "maidsafe/vault/demultiplexer.h"
#include "maidsafe/vault/pmid_account.h"
#include "maidsafe/vault/maid_account_pb.h"


namespace maidsafe {

namespace vault {

MaidAccount::MaidAccount(const MaidName& maid_name, const boost::filesystem::path& root)
    : kMaidName_(maid_name),
      pmid_totals_(),
      recent_put_data_(),
      total_data_stored_by_pmids_(0),
      total_put_data_(0),
      archive_(root / EncodeToBase32(kMaidName_.data)) {}

MaidAccount::MaidAccount(const serialised_type& serialised_maid_account,
                         const boost::filesystem::path& root)
    : kMaidName_([&serialised_maid_account]()->Identity {
                     protobuf::MaidAccount proto_maid_account;
                     proto_maid_account.ParseFromString(serialised_maid_account->string());
                     return Identity(proto_maid_account.maid_name());
                 }()),
      pmid_totals_(),
      recent_put_data_(),
      total_data_stored_by_pmids_(0),
      total_put_data_(0),
      archive_(root / EncodeToBase32(kMaidName_.data)) {
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

  for (int i(0); i != proto_maid_account.recent_put_data_size(); ++i) {
    auto& recent_put_data(proto_maid_account.recent_put_data(i));
    recent_put_data_.emplace(std::make_pair(
        GetDataNameVariant(recent_put_data.type(), Identity(recent_put_data.name())),
        PutDataDetails(recent_put_data.size(), recent_put_data.replication_count())));
  }

  total_data_stored_by_pmids_ = proto_maid_account.total_data_stored_by_pmids();
  total_put_data_ = proto_maid_account.total_put_data();
}

MaidAccount::serialised_type MaidAccount::Serialise() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return NonEmptyString(proto_maid_account_.SerializeAsString());
}

/*
void MaidAccount::PushPmidTotal(PmidTotals pmid_total) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_totals_.push_back(pmid_total);
}

void MaidAccount::RemovePmidTotal(Identity pmid_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto itr = pmid_totals_.begin(); itr != pmid_totals_.end(); ++itr) {
    if ((*itr).IsRecordOf(pmid_id)) {
      pmid_totals_.erase(itr);
      return;
    }
  }
}

void MaidAccount::UpdatePmidTotal(PmidTotals pmid_total) {
  RemovePmidTotal(pmid_total.pmid_id());
  PushPmidTotal(pmid_total);
}

bool MaidAccount::HasPmidTotal(Identity pmid_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto pmid_total_it = std::find_if(pmid_totals_.begin(), pmid_totals_.end(),
                                    [&pmid_id] (const PmidTotals& pmid_total) {
                                      return pmid_total.IsRecordOf(pmid_id);
                                    });
  return (pmid_total_it != pmid_totals_.end());
}

void MaidAccount::PushDataElement(DataElement data_element) {
  std::lock_guard<std::mutex> lock(mutex_);
  data_elements_.push_back(data_element);
}

void MaidAccount::RemoveDataElement(Identity name) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto itr = data_elements_.begin(); itr != data_elements_.end(); ++itr) {
    if ((*itr).name() == name) {
      data_elements_.erase(itr);
      return;
    }
  }
}

void MaidAccount::UpdateDataElement(DataElement data_element) {
  RemoveDataElement(data_element.name());
  PushDataElement(data_element);
}

bool MaidAccount::HasDataElement(Identity name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto data_element_it = std::find_if(data_elements_.begin(), data_elements_.end(),
                                      [&name] (const DataElement& data_element) {
                                        return data_element.name() == name;
                                      });
  return (data_element_it != data_elements_.end());
}
*/


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

}  // namespace vault

}  // namespace maidsafe
