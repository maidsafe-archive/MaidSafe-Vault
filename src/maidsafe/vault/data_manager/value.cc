/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/data_manager/value.h"

#include <string>

//#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {


DataManagerValue::DataManagerValue(const serialised_type& serialised_metadata_value)
  : subscribers_(),
    online_pmids_(),
    offline_pmids_() {
  protobuf::DataManagerValue metadata_value_proto;
  if (!metadata_value_proto.ParseFromString(serialised_metadata_value->string()) ||
      metadata_value_proto.size() != 0 ||
      metadata_value_proto.subscribers() < 1) {
    LOG(kError) << "Failed to read or parse serialised metadata value";
    ThrowError(CommonErrors::parsing_error);
  } else {
    if (metadata_value_proto.size() < 1) {
      LOG(kError) << "Invalid data size";
      ThrowError(CommonErrors::invalid_parameter);
    }

    if (metadata_value_proto.subscribers() < 1) {
      LOG(kError) << "Invalid subscribers count";
      ThrowError(CommonErrors::invalid_parameter);
    }
    subscribers_ = metadata_value_proto.subscribers();

    for (auto& i : metadata_value_proto.online_pmid_name())
      online_pmids_.insert(PmidName(Identity(i)));
    for (auto& i : metadata_value_proto.offline_pmid_name())
      offline_pmids_.insert(PmidName(Identity(i)));
    if (online_pmids_.size() + offline_pmids_.size() < 1) {
      LOG(kError) << "Invalid online/offline pmids";
      ThrowError(CommonErrors::invalid_parameter);
    }
  }
}

DataManagerValue::DataManagerValue()
    : subscribers_(0),
      online_pmids_(),
      offline_pmids_() {}


void DataManagerValue::AddPmid(const PmidName& pmid_name) {
  online_pmids_.insert(pmid_name);
  offline_pmids_.erase(pmid_name);
}

void DataManagerValue::RemovePmid(const PmidName& pmid_name) {
  if (online_pmids_.size() + offline_pmids_.size() < 4) {
    LOG(kError) << "RemovePmid not allowed";
    ThrowError(CommonErrors::invalid_parameter); // TODO add error - not_allowed
  }
  online_pmids_.erase(pmid_name);
  offline_pmids_.erase(pmid_name);
}

void DataManagerValue::IncrementSubscribers() {
  ++subscribers_;
}

int64_t DataManagerValue::DecrementSubscribers() {
  --subscribers_;
  return subscribers_;
}

void DataManagerValue::SetPmidOnline(const PmidName& pmid_name) {
  auto deleted = offline_pmids_.erase(pmid_name);
  if (deleted == 1) {
    online_pmids_.insert(pmid_name);
  } else {
    LOG(kError) << "Invalid Pmid reported";
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void DataManagerValue::SetPmidOffline(const PmidName& pmid_name) {
  auto deleted = online_pmids_.erase(pmid_name);
  if (deleted == 1) {
    offline_pmids_.insert(pmid_name);
  } else {
    LOG(kError) << "Invalid Pmid reported";
    ThrowError(CommonErrors::invalid_parameter);
  }
}

DataManagerValue::serialised_type DataManagerValue::Serialise() const {
  if (subscribers_ < 1)
    ThrowError(CommonErrors::uninitialised);  // Cannot serialise if not a complete db value
  assert((online_pmids_.size() + offline_pmids_.size()) > 0);
  protobuf::DataManagerValue metadata_value_proto;
  metadata_value_proto.set_subscribers(subscribers_);
  for (const auto& i: online_pmids_)
    metadata_value_proto.add_online_pmid_name(i->string());
  for (const auto& i: offline_pmids_)
    metadata_value_proto.add_offline_pmid_name(i->string());
  assert(metadata_value_proto.IsInitialized());
  return serialised_type(NonEmptyString(metadata_value_proto.SerializeAsString()));
}

bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs) {
  return lhs.subscribers_ == rhs.subscribers_ &&
         lhs.online_pmids_ == rhs.online_pmids_ &&
         lhs.offline_pmids_ == rhs.offline_pmids_;
}

}  // namespace vault

}  // namespace maidsafe
