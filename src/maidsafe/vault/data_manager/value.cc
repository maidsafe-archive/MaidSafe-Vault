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

#include <utility>

#include "maidsafe/common/utils.h"

#include "maidsafe/routing/parameters.h"

namespace maidsafe {

namespace vault {

DataManagerValue::DataManagerValue(const std::string &serialised_value)
    : size_(0), pmids_() {
  protobuf::DataManagerValue value_proto;
  if (!value_proto.ParseFromString(serialised_value)) {
    LOG(kError) << "Failed to read or parse serialised value";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  } else {
    if (value_proto.size() <= 0) {
      LOG(kError) << "Invalid parameters";
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
    }
    size_ = value_proto.size();
    for (auto& i : value_proto.pmid_names())
      pmids_.insert(PmidName(Identity(i)));
    if (pmids_.size() < 1) {
      LOG(kError) << "Invalid pmids";
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
    }
  }
}

DataManagerValue& DataManagerValue::operator=(const DataManagerValue& other) {
  size_ = other.size_;
  pmids_ = other.pmids_;
  return *this;
}

DataManagerValue::DataManagerValue(const DataManagerValue& other) {
  size_ = other.size_;
  pmids_ = other.pmids_;
}

DataManagerValue::DataManagerValue(const PmidName& pmid_name, int32_t size)
    : size_(size), pmids_() {
  AddPmid(pmid_name);
}

DataManagerValue::DataManagerValue(DataManagerValue&& other) MAIDSAFE_NOEXCEPT
    : size_(std::move(other.size_)), pmids_(std::move(other.pmids_)) {}

void DataManagerValue::AddPmid(const PmidName& pmid_name) {
  LOG(kVerbose) << "DataManagerValue::AddPmid adding " << HexSubstr(pmid_name->string());
  pmids_.insert(pmid_name);
//  PrintRecords();
}

void DataManagerValue::RemovePmid(const PmidName& pmid_name) {
  LOG(kVerbose) << "DataManagerValue::RemovePmid removing " << HexSubstr(pmid_name->string());
//  if (online_pmids_.size() + offline_pmids_.size() < 4) {
//    LOG(kError) << "RemovePmid not allowed";
//    // TODO add error - not_allowed
//    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
//  }
  pmids_.erase(pmid_name);
  PrintRecords();
}

bool DataManagerValue::HasTarget(const PmidName& pmid_name) const {
  for (auto& pmid : pmids_)
    if (pmid_name == pmid)
      return true;
  return false;
}

bool DataManagerValue::NeedToPrune(const PmidName& target,
                                   routing::Routing& routing,
                                   PmidName& pmid_node_to_remove) const {
  // Prune the furthest offline node
  if (pmids_.size() < routing::Parameters::closest_nodes_size)
    return false;

  std::vector<PmidName> offline_pmids;
  for (auto& pmid : pmids_)
    if (!routing.IsConnectedVault(NodeId(pmid->string())))
      offline_pmids.push_back(pmid);
  assert(!offline_pmids.empty());
  std::sort(offline_pmids.begin(), offline_pmids.end(),
            [&](const PmidName& lhs, const PmidName& rhs) {
    return NodeId::CloserToTarget(NodeId(lhs->string()), NodeId(rhs->string()),
                                  NodeId(target->string()));
  });
  pmid_node_to_remove = offline_pmids.back();
  return true;
}

std::string DataManagerValue::Serialise() const {
  if (size_ <= 0) {
    LOG(kError) << "DataManagerValue::Serialise Cannot serialise if not a complete db value";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::uninitialised));
  }
  assert(!pmids_.empty());
  protobuf::DataManagerValue value_proto;
  value_proto.set_size(size_);
  for (const auto& i : pmids_)
    value_proto.add_pmid_names(i->string());
  assert(value_proto.IsInitialized());
  return value_proto.SerializeAsString();
}

bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs) {
  return lhs.size_ == rhs.size_ && lhs.pmids_ == rhs.pmids_;
}

std::set<PmidName> DataManagerValue::online_pmids(routing::Routing& routing) const {
  std::set<PmidName> online_pmids;
  for (auto& pmid : pmids_)
    if (routing.IsConnectedVault(NodeId(pmid->string())))
      online_pmids.insert(pmid);
  return online_pmids;
}

void DataManagerValue::PrintRecords() {
  LOG(kVerbose) << "pmids_ now having : ";
  for (auto pmid : pmids_) {
    LOG(kVerbose) << "     ----     " << HexSubstr(pmid.value.string());
  }
}

std::string DataManagerValue::Print() const {
  std::stringstream stream;
  stream << "\n\t[size_," << size_ << "]";
  stream << "\n\t\t pmids_ now having : ";
  for (auto pmid : pmids_)
    stream << "\n\t\t     ----     " << HexSubstr(pmid.value.string());
  return stream.str();
}

DataManagerValue DataManagerValue::Resolve(const std::vector<DataManagerValue>& values) {
  std::vector<std::pair<DataManagerValue, unsigned int>> stats;
  auto max_iter(std::begin(stats));
  for (const auto& value : values) {
    auto iter(std::find_if(std::begin(stats), std::end(stats),
                           [&](const std::pair<DataManagerValue, unsigned int>& pair) {
                             return value == pair.first;
                           }));
    if (iter == std::end(stats))
      stats.emplace_back(std::make_pair(value, 0));
    else
      iter->second++;
    max_iter = (iter->second > max_iter->second) ? iter : max_iter;
  }

  if (max_iter->second == (routing::Parameters::group_size + 1) / 2)
    return max_iter->first;

  if (max_iter->second == routing::Parameters::group_size - 1)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));

  BOOST_THROW_EXCEPTION(MakeError(VaultErrors::too_few_entries_to_resolve));
}

}  // namespace vault

}  // namespace maidsafe
