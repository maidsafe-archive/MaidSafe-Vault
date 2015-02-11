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

#include "maidsafe/routing/types.h"

namespace maidsafe {

namespace vault {

class DataManagerDatabase {
 public:
  template <typename DataType>
  bool Exist(typename DataType::Name /*name*/) {
    return true;
  }

  template <typename DataType>
  void Put(typename DataType::Name /*name*/, std::vector<routing::Address> /*pmid_nodes*/,
           u_int64_t /*size*/) {}

  template <typename DataType>
  void RemovePmid(typename DataType::Name /*name*/, routing::Address /*address*/) {}

  template <typename DataType>
  std::vector<routing::Address> GetPmids(typename DataType::Name /*name*/) {
    return std::vector<routing::Address>();
  }
};

namespace detail {
  template <typename DataType>
  std::vector<routing::DestinationAddress> GetClosestNodes(
      typename DataType::Name /*name*/, std::vector<routing::Address> /*exclude*/) {
    return std::vector<routing::DestinationAddress>();
  }

  class Parameters {
    static size_t min_holders;
  };

  size_t Parameters::min_holders = 4;
}

template <typename FacadeType>
class DataManager {
 public:
  DataManager() {}

  template <typename DataType>
  routing::HandleGetReturn HandleGet(Identity name);

  template <typename DataType>
  routing::HandlePutPostReturn HandlePut(DataType data);

  template <typename DataType>
  routing::HandlePutPostReturn
  HandlePutResponse(typename DataType::Name name, routing::DestinationAddress from,
                    maidsafe_error return_code);

  void HandleChurn(routing::CloseGroupDifference);

 private:
  template <typename DataType>
  routing::HandlePutPostReturn Replicate(typename DataType::Name name,
                                         routing::Address exclude = routing::Address());

  void DownRank(routing::Address /*address*/) {}

  DataManagerDatabase db_;
  routing::CloseGroupDifference close_group_;
};

//template <typename FacadeType>
//DataManager<FacadeType>::DataManager(boost::filesystem::path vault_root_dir)
//    : db_(UniqueDbPath(vault_root_dir)), close_group_() {}

template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn DataManager<FacadeType>::HandlePut(DataType data) {
  if (!db_.Exist(data)) {
    auto pmid_nodes(detail::GetClosestNodes(data.name()));
    db_.Put(data.name(), pmid_nodes, data.size());
    return boost::make_expected(pmid_nodes);
  }
  return boost::make_unexpected(CommonErrors::success);
}

template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn DataManager<FacadeType>::HandlePutResponse(
    typename DataType::Name name, routing::DestinationAddress from,
    maidsafe_error return_code) {
  if (return_code.code() == make_error_code(CommonErrors::success)) {
    return boost::make_unexpected(CommonErrors::success);
  } else {
    try {
      db_.RemovePmid(name, from);
    }
    catch (maidsafe_error error) {
      if (error.code() == make_error_code(CommonErrors::no_such_element))
        return boost::make_unexpected(CommonErrors::no_such_element);
      else
        throw;
    }
  }

  DownRank(from);  // failed to store
  return Replicate(name, from);
}

template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn
DataManager<FacadeType>::Replicate(typename DataType::Name name, routing::Address tried_pmid_node) {
  std::vector<routing::Address> current_pmid_nodes;
  try {
    current_pmid_nodes = db_.GetPmids(name);
    if (tried_pmid_node != routing::Address())
      current_pmid_nodes.push_back(tried_pmid_node);
  }
  catch (maidsafe_error error) {
    if (error.code() == make_error_code(CommonErrors::no_such_element))
      return boost::make_unexpected(CommonErrors::unable_to_handle_request);

    throw;
  }
  if (current_pmid_nodes.size() >= detail::Parameters::min_holders)
    return boost::make_unexpected(CommonErrors::success);

  auto pmid_nodes(detail::GetClosestNodes(name, current_pmid_nodes));
  if (pmid_nodes.empty()) {
    LOG(kError) << "Failed to find a valid close pmid node";
    return boost::make_unexpected(CommonErrors::unable_to_handle_request);
  }
  return boost::make_expected(pmid_nodes);
}

template <typename FacadeType>
template <typename DataType>
routing::HandleGetReturn DataManager<FacadeType>::HandleGet(Identity name) {
  std::vector<routing::Address> pmid_nodes;
  try {
    pmid_nodes = db_.GetPmids<DataType>(name);
  }
  catch (maidsafe_error error) {
    if (error.code() == make_error_code(CommonErrors::no_such_element))
      return boost::make_unexpected(CommonErrors::no_such_element);
    throw;
  }
  return boost::make_expected(pmid_nodes);
}

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_DATA_MANAGER_H_
