/*  Copyright 2015 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_H_

#include "maidsafe/common/types.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/data_manager/database.h"

namespace maidsafe {

namespace vault {

template <typename FacadeType>
class DataManager {
 public:
  DataManager(const boost::filesystem::path& vault_root_dir);

  template <typename DataType>
  routing::HandleGetReturn HandleGet(const routing::SourceAddress& from, const Identity& name);

  template <typename DataType>
  routing::HandlePutPostReturn HandlePut(const routing::SourceAddress& from,
                                         const DataType& data);

  template <typename DataType>
  routing::HandlePutPostReturn
  HandlePutResponse(const typename DataType::Name& name, const routing::DestinationAddress& from,
                    const maidsafe_error& return_code);

  void HandleChurn(const routing::CloseGroupDifference& difference);

 private:
  template <typename DataType>
  routing::HandlePutPostReturn Replicate(const typename DataType::Name& name,
                                         const routing::Address& exclude);

  void DownRank(const routing::Address& /*address*/) {}

  DataManagerDatabase db_;
  routing::CloseGroupDifference close_group_;
};

template <typename FacadeType>
DataManager<FacadeType>::DataManager(const boost::filesystem::path& vault_root_dir)
    : db_(UniqueDbPath(vault_root_dir)) {}

template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn DataManager<FacadeType>::HandlePut(
    const routing::SourceAddress& /*from*/, const DataType& data) {
  if (!db_.Exist<DataType>(data.name())) {
    auto pmid_addresses(GetClosestNodes<DataType>(data.name()));
    db_.Put<DataType>(data.name(), pmid_addresses);
    std::vector<routing::DestinationAddress> dest_addresses;
    for (const auto& pmid_address : pmid_addresses)
      dest_addresses.emplace_back(std::make_pair(routing::Destination(pmid_address),
                                                 boost::none));
    return dest_addresses;
  }
  return boost::make_unexpected(MakeError(CommonErrors::success));
}

template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn DataManager<FacadeType>::HandlePutResponse(
    const typename DataType::Name& name, const routing::DestinationAddress& from,
    const maidsafe_error& return_code) {
  assert(return_code.code() != make_error_code(CommonErrors::success));
  DownRank(from);  // failed to store
  return Replicate(name, from);
}

template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn
DataManager<FacadeType>::Replicate(const typename DataType::Name& name,
                                   const routing::Address& from) {
  std::vector<routing::Address> current_pmid_nodes, new_pmid_nodes;
  bool is_holder(false);
  current_pmid_nodes = db_.GetPmids(name).value();
  is_holder = (std::any_of(current_pmid_nodes.begin(), current_pmid_nodes.end(),
                           [&](routing::Address pmid) { return pmid == from; }));
  if (current_pmid_nodes.size() > Parameters::min_pmid_holders) {
    if (is_holder)
      db_.RemovePmid(name, from);
    return boost::make_unexpected(MakeError(CommonErrors::success));
  }

  new_pmid_nodes = GetClosestNodes(name, current_pmid_nodes);
  if (new_pmid_nodes.empty()) {
    if (is_holder)
      db_.RemovePmid(name, from);
    LOG(kError) << "Failed to find a valid close pmid node";
    return boost::make_unexpected(MakeError(CommonErrors::unable_to_handle_request));
  }
  current_pmid_nodes.erase(std::remove(current_pmid_nodes.begin(), current_pmid_nodes.end(),
                                       from),
                           current_pmid_nodes.end());
  current_pmid_nodes.insert(current_pmid_nodes.end(), new_pmid_nodes.begin(),
                            new_pmid_nodes.end());
  db_.ReplacePmidNodes(name, current_pmid_nodes);

  std::vector<routing::DestinationAddress> dest_addresses;
  for (const auto& pmid_address : new_pmid_nodes)
    dest_addresses.emplace_back(std::make_pair(routing::Destination(pmid_address), boost::none));
  return dest_addresses;
}

template <typename FacadeType>
template <typename DataType>
routing::HandleGetReturn DataManager<FacadeType>::HandleGet(const routing::SourceAddress& from,
                                                            const Identity& name) {
  DataManagerDatabase::GetPmidsResult result;
  result = db_.GetPmids<DataType>(typename DataType::Name(name));
  if (!result.valid())
    return boost::make_unexpected(MakeError(CommonErrors::no_such_element));
  if (result.value().empty())
    return boost::make_unexpected(MakeError(CommonErrors::unable_to_handle_request));

  std::vector<routing::DestinationAddress> dest_pmids;
  for (const auto& holder : *result)
    dest_pmids.emplace_back(routing::Destination(holder),
                            boost::optional<routing::ReplyToAddress>(from.node_address.data));
  return routing::HandleGetReturn::value_type(dest_pmids);
}

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_DATA_MANAGER_H_
