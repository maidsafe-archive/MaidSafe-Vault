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

#ifndef MAIDSAFE_VAULT_TESTS_VAULT_NETWORK_H_
#define MAIDSAFE_VAULT_TESTS_VAULT_NETWORK_H_

#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/vault/vault.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace test {

const int kNetworkSize(50);

class VaultNetwork : public testing::Test{
 public:
  typedef std::shared_ptr<Vault> VaultPtr;

  VaultNetwork();
  void Bootstrap();
  virtual void TearDown();
  void Create(size_t index);

 protected:
  AsioService asio_service_;
  std::mutex mutex_;  
  std::condition_variable bootstrap_condition_, network_up_condition_;
  bool bootstrap_done_, network_up_;
  std::vector<VaultPtr> vaults_;
  std::vector<boost::asio::ip::udp::endpoint> endpoints_;
  std::vector<passport::PublicPmid> public_pmids_;
  std::vector<passport::Pmid> pmids_;
  fs::path chunk_store_path_;
  size_t network_size_;
};

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TESTS_VAULT_NETWORK_H_
