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

namespace maidsafe {

namespace vault {

DataManagerValue::DataManagerValue(const std::string &serialised_metadata_value)
    : subscribers_(0), size_(0), online_pmids_(), offline_pmids_() {
  protobuf::DataManagerValue metadata_value_proto;
  if (!metadata_value_proto.ParseFromString(serialised_metadata_value)) {
    LOG(kError) << "Failed to read or parse serialised metadata value";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  } else {
    if ((metadata_value_proto.subscribers() < 1) || (metadata_value_proto.size() <= 0)) {
      LOG(kError) << "Invalid parameters";
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
    }
    subscribers_ = metadata_value_proto.subscribers();
    size_ = metadata_value_proto.size();

    for (auto& i : metadata_value_proto.online_pmid_name())
      online_pmids_.insert(PmidName(Identity(i)));
    for (auto& i : metadata_value_proto.offline_pmid_name())
      offline_pmids_.insert(PmidName(Identity(i)));
    if (online_pmids_.size() + offline_pmids_.size() < 1) {
      LOG(kError) << "Invalid online/offline pmids";
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
    }
  }
}

DataManagerValue& DataManagerValue::operator=(const DataManagerValue& other) {
  subscribers_ = other.subscribers_;
  size_ = other.size_;
  online_pmids_ = other.online_pmids_;
  offline_pmids_ = other.offline_pmids_;
  return *this;
}

DataManagerValue::DataManagerValue(const PmidName& pmid_name, int32_t size)
    : subscribers_(0), size_(size), online_pmids_(), offline_pmids_() {
  AddPmid(pmid_name);
}

DataManagerValue::DataManagerValue(DataManagerValue&& other)
    : subscribers_(std::move(other.subscribers_)),
      size_(std::move(other.size_)),
      online_pmids_(std::move(other.online_pmids_)),
      offline_pmids_(std::move(other.offline_pmids_)) {}

void DataManagerValue::AddPmid(const PmidName& pmid_name) {
  LOG(kVerbose) << "DataManagerValue::AddPmid adding " << HexSubstr(pmid_name->string());
  online_pmids_.insert(pmid_name);
  offline_pmids_.erase(pmid_name);
  PrintRecords();
}

void DataManagerValue::RemovePmid(const PmidName& pmid_name) {
  LOG(kVerbose) << "DataManagerValue::RemovePmid removing " << HexSubstr(pmid_name->string());
//  if (online_pmids_.size() + offline_pmids_.size() < 4) {
//    LOG(kError) << "RemovePmid not allowed";
//    // TODO add error - not_allowed
//    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
//  }
  online_pmids_.erase(pmid_name);
  offline_pmids_.erase(pmid_name);
  PrintRecords();
}

int64_t DataManagerValue::DecrementSubscribers() {
  --subscribers_;
  LOG(kGraph) << "DataManager decrease subscribers to " << subscribers_;
  return subscribers_;
}

void DataManagerValue::SetPmidOnline(const PmidName& pmid_name) {
  LOG(kVerbose) << "DataManagerValue::SetPmidOnline " << HexSubstr(pmid_name->string());
  auto deleted = offline_pmids_.erase(pmid_name);
  if (deleted == 1) {
    online_pmids_.insert(pmid_name);
  } else {
    LOG(kError) << "Invalid Pmid reported";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  PrintRecords();
}

void DataManagerValue::SetPmidOffline(const PmidName& pmid_name) {
  LOG(kVerbose) << "DataManagerValue::SetPmidOffline " << HexSubstr(pmid_name->string());
  auto deleted = online_pmids_.erase(pmid_name);
  if (deleted == 1) {
    offline_pmids_.insert(pmid_name);
  } else {
    LOG(kError) << "Invalid Pmid reported";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  PrintRecords();
}

std::string DataManagerValue::Serialise() const {
  if (subscribers_ < 1 || size_ <= 0) {
    LOG(kError) << "DataManagerValue::Serialise Cannot serialise if not a complete db value";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::uninitialised));
  }
  assert(!(online_pmids_.empty() && offline_pmids_.empty()));
  protobuf::DataManagerValue metadata_value_proto;
  metadata_value_proto.set_subscribers(subscribers_);
  metadata_value_proto.set_size(size_);
  for (const auto& i : online_pmids_)
    metadata_value_proto.add_online_pmid_name(i->string());
  for (const auto& i : offline_pmids_)
    metadata_value_proto.add_offline_pmid_name(i->string());
  assert(metadata_value_proto.IsInitialized());
  return metadata_value_proto.SerializeAsString();
}

bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs) {
  return lhs.subscribers_ == rhs.subscribers_ && lhs.size_ == rhs.size_ &&
         lhs.online_pmids_ == rhs.online_pmids_ && lhs.offline_pmids_ == rhs.offline_pmids_;
}

std::set<PmidName> DataManagerValue::AllPmids() const {
  std::set<PmidName> pmids_union;
  std::set_union(std::begin(online_pmids_), std::end(online_pmids_), std::begin(offline_pmids_),
                 std::end(offline_pmids_), std::inserter(pmids_union, std::begin(pmids_union)));
  return pmids_union;
}

void DataManagerValue::PrintRecords() {
  LOG(kVerbose) << " online_pmids_ now having : ";
  for (auto pmid : online_pmids_) {
    LOG(kVerbose) << "     ----     " << HexSubstr(pmid.value.string());
  }
  LOG(kVerbose) << " offline_pmids_ now having : ";
  for (auto pmid : offline_pmids_) {
    LOG(kVerbose) << "     ----     " << HexSubstr(pmid.value.string());
  }
}

}  // namespace vault

}  // namespace maidsafe
