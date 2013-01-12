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

#include "maidsafe/common/utils.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account.h"
#include "maidsafe/vault/maid_account_holder/maid_account_pb.h"


namespace maidsafe {

namespace vault {

void MaidAccount::Parse(const NonEmptyString& serialised_maidaccount) {
  protobuf::MaidAccount proto_maid_account;
  if (!proto_maid_account.ParseFromString(serialised_maidaccount.string()) ||
      !proto_maid_account.IsInitialized()) {
    LOG(kError) << "Failed to parse maid_account.";
    ThrowError(NfsErrors::maid_account_parsing_error);
  }

  std::lock_guard<std::mutex> lock(mutex_);
  maid_name_.data = Identity(proto_maid_account.maid_name());

  data_elements_.clear();
  for (int i(0); i != proto_maid_account.total_data_put_to_network(); ++i) {
    data_elements_.push_back(DataElement(
        Identity(proto_maid_account.data_put_to_network(i).name()),
          proto_maid_account.data_put_to_network(i).size()));
  }

  pmid_totals_.clear();
  for (int i(0); i != proto_maid_account.total_data_put_to_network(); ++i) {
    pmid_totals_.push_back(
        PmidTotal(nfs::PmidRegistration(NonEmptyString(
                      proto_maid_account.pmid_totals(i).serialised_pmid_registration())),
                  PmidRecord(NonEmptyString(
                      proto_maid_account.pmid_totals(i).pmid_record().SerializeAsString()))));
  }
}

NonEmptyString MaidAccount::Serialise() {
  std::lock_guard<std::mutex> lock(mutex_);
  protobuf::MaidAccount proto_maid_account;
  proto_maid_account.set_maid_name(maid_name_.data.string());
  for (auto& pmid_total : pmid_totals_) {
    protobuf::PmidTotals proto_pmid_total;
    proto_pmid_total.ParseFromString(pmid_total.Serialise().string());
    *(proto_maid_account.add_pmid_totals()) = proto_pmid_total;
  }
  for (auto& data_element : data_elements_) {
    protobuf::DataElements proto_data_elements;
    proto_data_elements.ParseFromString(data_element.Serialise().string());
    *(proto_maid_account.add_data_put_to_network()) = proto_data_elements;
  }
  return NonEmptyString(proto_maid_account.SerializeAsString());
}

MaidAccount& MaidAccount::operator=(const MaidAccount& other) {
  maid_name_ = other.maid_name_;
  pmid_totals_ = other.pmid_totals_;
  data_elements_ = other.data_elements_;
  return *this;
}

void MaidAccount::PushPmidTotal(PmidTotal pmid_total) {
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

void MaidAccount::UpdatePmidTotal(PmidTotal pmid_total) {
  RemovePmidTotal(pmid_total.pmid_id());
  PushPmidTotal(pmid_total);
}

bool MaidAccount::HasPmidTotal(Identity pmid_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto pmid_total_it = std::find_if(pmid_totals_.begin(), pmid_totals_.end(),
                                    [&pmid_id] (const PmidTotal& pmid_total) {
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

}  // namespace vault

}  // namespace maidsafe
