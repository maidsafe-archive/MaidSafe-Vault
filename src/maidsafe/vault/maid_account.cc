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

#include "maidsafe/vault/pmid_account.h"
#include "maidsafe/vault/maid_account_pb.h"


namespace maidsafe {

namespace vault {

MaidAccount::MaidAccount(const MaidName& maid_name)
    : mutex_(),
      proto_maid_account_(),
      kMaidName_(maid_name) {}

MaidAccount::MaidAccount(const NonEmptyString& serialised_maid_account)
    : mutex_(),
      proto_maid_account_([&serialised_maid_account]()->protobuf::MaidAccount {
                              protobuf::MaidAccount proto_maid_account;
                              proto_maid_account.ParseFromString(serialised_maid_account.string());
                              return proto_maid_account;
                          }()),
      kMaidName_(Identity(proto_maid_account_.maid_name())) {
  if (!proto_maid_account_.IsInitialized()) {
    LOG(kError) << "Failed to parse maid_account.";
    ThrowError(NfsErrors::maid_account_parsing_error);
  }
}

NonEmptyString MaidAccount::Serialise() const {
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

}  // namespace vault

}  // namespace maidsafe
