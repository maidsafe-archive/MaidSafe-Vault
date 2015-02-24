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

#ifndef MAIDSAFE_VAULT_PMID_NODE_H_
#define MAIDSAFE_VAULT_PMID_NODE_H_

#include "maidsafe/common/types.h"
#include "maidsafe/routing/types.h"

#include "maidsafe/vault/chunk_store.h"


namespace maidsafe {

namespace vault {


template <typename FacadeType>
class PmidNode {
 public:
  PmidNode(const boost::filesystem::path vault_root_dir, DiskUsage max_disk_usage);
  template <typename DataType>
  routing::HandleGetReturn HandleGet(routing::SourceAddress from, Identity data_name);

  template <typename DataType>
  routing::HandlePutPostReturn HandlePut(routing::SourceAddress from, DataType data);
  void HandleChurn(routing::CloseGroupDifference);

 private:
  boost::filesystem::space_info space_info_;
  DiskUsage disk_total_;
  DiskUsage permanent_size_;
  ChunkStore chunk_store_;
};

template <typename FacadeType>
PmidNode<FacadeType>::PmidNode(const boost::filesystem::path vault_root_dir, DiskUsage max_disk_usage)
    : space_info_(boost::filesystem::space(vault_root_dir)),
      disk_total_(space_info_.available),
      permanent_size_(disk_total_ * 4 / 5),
      chunk_store_(vault_root_dir / "pmid_node" / "permanent", max_disk_usage) {}

template <typename FacadeType>
template <typename DataType>
routing::HandleGetReturn PmidNode<FacadeType>::HandleGet(routing::SourceAddress /*from*/,
                                                         Identity data_name) {
  const typename DataType::Name NameVariant(data_name);
  DataNameVariant data_name_variant(NameVariant);
  try {
    auto deobfuscated_data(chunk_store_.Get(data_name_variant));

    return routing::HandleGetReturn::value_type(
                std::vector<byte>(std::begin(deobfuscated_data.string()),
                                  std::end(deobfuscated_data.string())));
  } catch (const std::exception& e) {
    return boost::make_unexpected(MakeError(CommonErrors::no_such_element));
  }
}

template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn PmidNode<FacadeType>::HandlePut(routing::SourceAddress /* from */,
                                                             DataType /* data */) {
  return boost::make_unexpected(MakeError(VaultErrors::failed_to_handle_request));  // FIXME
}

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_PMID_NODE_H_
