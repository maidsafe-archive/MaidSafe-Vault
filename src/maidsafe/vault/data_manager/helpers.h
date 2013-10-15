/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_HELPERS_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_HELPERS_H_

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

#include "maidsafe/common/config.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/vault/messages.h"

#include "maidsafe/vault/integrity_check_data.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace detail {

template <typename DataName, typename RequestorIdType>
struct GetResponseOp {
  GetResponseOp(PmidName pmid_node_to_get_from_in,
                nfs::MessageId message_id_in,
                std::map<PmidName, IntegrityCheckData> integrity_checks_in,
                DataName data_name_in,
                RequestorIdType requestor_id_in)
      : mutex(),
        message_id(std::move(message_id_in)),
        pmid_node_to_get_from(std::move(pmid_node_to_get_from_in)),
        integrity_checks(std::move(integrity_checks_in)),
        data_name(std::move(data_name_in)),
        requestor_id(std::move(requestor_id_in)),
        serialised_contents() {}

  std::mutex mutex;
  nfs::MessageId message_id;
  PmidName pmid_node_to_get_from;
  std::map<PmidName, IntegrityCheckData> integrity_checks;
  DataName data_name;
  RequestorIdType requestor_id;
  int called_count;
  typename DataName::data_type::serialised_type serialised_contents;
};

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_HELPERS_H_
