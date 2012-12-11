/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_VAULT_H_
#define MAIDSAFE_VAULT_VAULT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/thread/mutex.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/rsa.h"

#include "maidsafe/pd/client/node.h"

namespace maidsafe {

namespace priv { namespace chunk_store { struct ChunkData; } }

enum class RpcMessageType;

namespace protobuf {

namespace sync {

class GetSyncDataRequest;
class GetSyncDataResponse;

}  // namespace sync

namespace admin {

class GetPropertiesRequest;
class GetPropertiesResponse;
class ExecuteRequest;
class ExecuteResponse;

}  // namespace admin

}  // namespace protobuf

namespace test {

class PdTest;
class PdTest_FUNC_ChunkCorruption_Test;
class PdTest_FUNC_ChunkDeletion_For_Multiple_Clients_Test;
class PdTest_FUNC_ChunkDeletion_Test;
class PdTest_FUNC_ChunkSharing_Test;
class PdTest_FUNC_StoreAndGetChunks_Test;
class PdTest_FUNC_SynchronisationAndPruning_Test;

}  // namespace test

namespace vault {

class AbaService;
class AccountHandler;
class AccountService;
class ChunkInfoHandler;
class ChunkInfoService;
class ChunkIntegrityManager;
class ChunkService;

namespace test {

class NodeTest_BEH_Init_Test;
class NodeTest_BEH_Start_Stop_Test;
class NodeTest_BEH_AnnounceChunks_Test;
class NodeTest_BEH_GetProperties_Test;
class NodeTest_BEH_Execute_Test;
class NodeTest_BEH_OnMessageReceived_Test;
class NodeTest_BEH_OnCloseNodeReplaced_Test;
class NodeTest_BEH_OnStoreCacheData_Test;
}

#ifndef __APPLE__
class VaultStats;
#endif

class Node : public pd::Node {
 public:
  enum class NodeState {
    kStopped,
    kStarting,
    kStarted,
    kStopping
  };

  Node();
  ~Node();

  /// Initialises chunk storage and networking objects.
  int Start(const boost::filesystem::path &chunk_path,
            const std::vector<boost::asio::ip::udp::endpoint> &peer_endpoints =
                std::vector<boost::asio::ip::udp::endpoint>());

  /// Cancels pending operations and leaves the network.
  int Stop();

  void set_vault_stats_needed(bool vault_stats_needed,
                              const boost::filesystem::path &vault_plugins) {
    vault_stats_needed_ = vault_stats_needed;
    plugins_path_ = vault_plugins;
  }
  void set_do_prune_expired_data(bool value) { do_prune_expired_data_ = value; }
  void set_do_backup_state(bool value) { do_backup_state_ = value; }
  void set_do_synchronise(bool value) { do_synchronise_ = value; }
  void set_do_check_integrity(bool value) { do_check_integrity_ = value; }
  void set_do_announce_chunks(bool value) { do_announce_chunks_ = value; }
  void set_do_update_account(bool value) { do_update_account_ = value; }

#ifndef __APPLE__
  std::shared_ptr<VaultStats> vault_stats() const { return vault_stats_; }
#endif

 private:
  Node &operator=(const Node&);
  Node(const Node&);

  friend class pd::test::PdTest;
  friend class pd::test::PdTest_FUNC_ChunkCorruption_Test;
  friend class pd::test::PdTest_FUNC_ChunkDeletion_For_Multiple_Clients_Test;
  friend class pd::test::PdTest_FUNC_ChunkDeletion_Test;
  friend class pd::test::PdTest_FUNC_ChunkSharing_Test;
  friend class pd::test::PdTest_FUNC_StoreAndGetChunks_Test;
  friend class pd::test::PdTest_FUNC_SynchronisationAndPruning_Test;
  friend class test::NodeTest_BEH_Start_Stop_Test;
  friend class test::NodeTest_BEH_AnnounceChunks_Test;
  friend class test::NodeTest_BEH_GetProperties_Test;
  friend class test::NodeTest_BEH_Execute_Test;
  friend class test::NodeTest_BEH_OnMessageReceived_Test;
  friend class test::NodeTest_BEH_OnCloseNodeReplaced_Test;
  friend class test::NodeTest_BEH_OnStoreCacheData_Test;

  void PrepareServices();

  void OnMessageReceived(const RpcMessageType &message_type,
                         const std::string &message,
                         const Contact &sender,
                         const NodeId &group_claim,
                         const SecurityType &security_type,
                         bool cache_lookup,
                         const ReplyFunctor &reply_functor);

  void OnCloseNodeReplaced(const std::vector<NodeId> &new_close_nodes);

  void OnStoreCacheData(const RpcMessageType &message_type, const std::string &message);

  void GetSyncDataCallback(bool result, const std::multimap<std::string, std::string> &sync_data);

  void PerformMaintenance();
  void BackupState();
  void RestoreState();
  void AnnounceChunk();
  void UpdateAccount(bool retry, const std::function<void(bool)> &callback);  // NOLINT

  void GetSyncData(const protobuf::sync::GetSyncDataRequest &request,
                   protobuf::sync::GetSyncDataResponse *response);
  void GetProperties(const protobuf::admin::GetPropertiesRequest &request,
                     protobuf::admin::GetPropertiesResponse *response);
  void Execute(const protobuf::admin::ExecuteRequest &request,
               protobuf::admin::ExecuteResponse *response);

  NodeState state_;
  bool vault_stats_needed_, do_prune_expired_data_, do_backup_state_, do_synchronise_,
       do_check_integrity_, do_announce_chunks_, do_update_account_;
  boost::filesystem::path plugins_path_;
  std::map<NonEmptyString, crypto::TigerHash> last_backup_hashes_;
  std::vector<NodeId> prev_close_nodes_;
  int64_t account_capacity_;
  boost::posix_time::ptime start_time_;
  boost::mutex mutex_;
  std::vector<priv::chunk_store::ChunkData> local_chunks_;
  std::shared_ptr<ChunkInfoHandler> chunk_info_handler_;
  std::shared_ptr<AccountHandler> account_handler_;
#ifndef __APPLE__
  std::shared_ptr<VaultStats> vault_stats_;
#endif
  std::shared_ptr<ChunkService> chunk_service_;
  std::shared_ptr<ChunkInfoService> chunk_info_service_;
  std::shared_ptr<AccountService> account_service_;
  std::shared_ptr<AbaService> aba_service_;
  std::shared_ptr<ChunkIntegrityManager> chunk_integrity_manager_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
