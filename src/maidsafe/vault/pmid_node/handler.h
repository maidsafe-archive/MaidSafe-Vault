/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_PMID_NODE_HANDLER_H_
#define MAIDSAFE_VAULT_PMID_NODE_HANDLER_H_


#include "maidsafe/data_store/data_store.h"
#include "maidsafe/data_store/memory_buffer.h"
#include "maidsafe/data_store/permanent_store.h"


namespace maidsafe {
namespace vault {

class PmidNodeHandler {
 public:
  PmidNodeHandler();

  template<typename Data>
  void Put(const Data& data);

  data_store::PermanentStore permanent_data_store_;
  data_store::DataStore<data_store::DataBuffer> cache_data_store_;
  data_store::MemoryBuffer mem_only_cache_;
};

}  // namespace vault
}  // namespace maidsafe

#include "maidsafe/vault/pmid_manager/handler-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_NODE_HANDLER_H_
