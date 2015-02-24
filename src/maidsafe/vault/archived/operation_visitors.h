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

#ifndef MAIDSAFE_VAULT_OPERATION_VISITORS_H_
#define MAIDSAFE_VAULT_OPERATION_VISITORS_H_

#include <set>
#include <string>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/key.h"
#include "maidsafe/vault/version_handler/service.h"

namespace maidsafe {

namespace vault {

class VersionHandlerService;

namespace detail {

template <typename ServiceHandlerType>
class MaidManagerPutVisitor : public boost::static_visitor<> {
 public:
  MaidManagerPutVisitor(ServiceHandlerType* service, NonEmptyString content, NodeId sender,
                        nfs::MessageId message_id)
      : kService_(service),
        kContent_(std::move(content)),
        kSender_(std::move(sender)),
        kMessageId_(std::move(message_id)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    LOG(kVerbose) << "MaidManagerPutVisitor HandlePut for chunk "
                  << HexSubstr(data_name.value.string());
    kService_->HandlePut(
        MaidName(Identity(kSender_.string())),
        typename Name::data_type(data_name, typename Name::data_type::serialised_type(kContent_)),
        kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString kContent_;
  const NodeId kSender_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType, typename RequestorIdType>
class DataManagerPutVisitor : public boost::static_visitor<> {
 public:
  DataManagerPutVisitor(ServiceHandlerType* service, NonEmptyString content,
                        RequestorIdType requestor, nfs::MessageId message_id)
      : kService_(service),
        kContent_(std::move(content)),
        kRequestorId_(std::move(requestor)),
        kMessageId_(std::move(message_id)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandlePut(
        typename Name::data_type(data_name, typename Name::data_type::serialised_type(kContent_)),
        kRequestorId_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString kContent_;
  const RequestorIdType kRequestorId_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class DataManagerSendPutRequestVisitor : public boost::static_visitor<> {
 public:
  DataManagerSendPutRequestVisitor(ServiceHandlerType* const service, const PmidName& pmid_name,
                                   const NonEmptyString& data, nfs::MessageId message_id)
      : kService_(service), kPmidName_(pmid_name), kData_(data), kMessageId_(message_id) {}

  template<typename DataName>
  void operator()(const DataName& name) {
    using Data = typename DataName::data_type;
    kService_->template HandleSendPutRequest<Data>(
        kPmidName_, Data(name, typename Data::serialised_type(kData_)), kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const NonEmptyString kData_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class DataManagerGetForReplicationVisitor : public boost::static_visitor<> {
 public:
  DataManagerGetForReplicationVisitor(ServiceHandlerType* service,
                                      const std::set<PmidName>& online_pmids)
      : kService_(service), online_pmids_(online_pmids) {}

  DataManagerGetForReplicationVisitor(ServiceHandlerType* service,
                                      const std::vector<PmidName>& online_pmids)
      : kService_(service), online_pmids_() {
    std::for_each(std::begin(online_pmids), std::end(online_pmids),
                  [&](const PmidName& pmid_node) {
                    online_pmids_.insert(pmid_node);
                  });
  }

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template DoGetForReplication<typename Name::data_type>(data_name, online_pmids_);
  }

 private:
  ServiceHandlerType* const kService_;
  std::set<PmidName> online_pmids_;
};

template <typename ServiceHandlerType>
class PmidManagerPutVisitor : public boost::static_visitor<> {
 public:
  PmidManagerPutVisitor(ServiceHandlerType* service, const NonEmptyString& content,
                        const Identity& pmid_name, nfs::MessageId message_id)
      : kService_(service), kContent_(content), kPmidName_(pmid_name), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandlePut(
        typename Name::data_type(data_name, typename Name::data_type::serialised_type(kContent_)),
        kPmidName_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString kContent_;
  const PmidName kPmidName_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PmidNodePutVisitor : public boost::static_visitor<> {
 public:
  PmidNodePutVisitor(ServiceHandlerType* service, const NonEmptyString& content,
                     nfs::MessageId message_id)
      : kService_(service), kContent_(content), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandlePut(
        typename Name::data_type(data_name, typename Name::data_type::serialised_type(kContent_)),
        kContent_.string().size(), kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString kContent_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PutResponseFailureVisitor : public boost::static_visitor<> {
 public:
  PutResponseFailureVisitor(ServiceHandlerType* service, uint64_t size, const Identity& pmid_node,
                            const maidsafe_error& return_code, nfs::MessageId message_id)
      : kService_(service),
        kSize_(size),
        kPmidNode_(pmid_node),
        kReturnCode_(return_code),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutFailure<typename Name::data_type>(
        data_name, kSize_, kPmidNode_, kMessageId_, maidsafe_error(kReturnCode_));
  }

 private:
  ServiceHandlerType* const kService_;
  const uint64_t kSize_;
  const PmidName kPmidNode_;
  const maidsafe_error kReturnCode_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class MaidManagerPutResponseFailureVisitor : public boost::static_visitor<> {
 public:
  MaidManagerPutResponseFailureVisitor(ServiceHandlerType* service, const MaidName& maid_node,
                                       const maidsafe_error& return_code, nfs::MessageId message_id)
      : kService_(service),
        kMaidNode_(maid_node),
        kReturnCode_(return_code),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutFailure<typename Name::data_type>(kMaidNode_, data_name,
                                                                   kReturnCode_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidNode_;
  const maidsafe_error kReturnCode_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PutResponseSuccessVisitor : public boost::static_visitor<> {
 public:
  PutResponseSuccessVisitor(ServiceHandlerType* service, const Identity& pmid_node,
                            nfs::MessageId message_id)
      : kService_(service), kPmidNode_(pmid_node), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutResponse<typename Name::data_type>(data_name, kPmidNode_,
                                                                    kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidNode_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType, typename AccountType>
class PutResponseVisitor : public boost::static_visitor<> {
 public:
  PutResponseVisitor(ServiceHandlerType* service, const Identity& account, int32_t cost,
                     nfs::MessageId message_id)
      : kService_(service), kAccountName_(account), kCost_(cost), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutResponse<typename Name::data_type>(kAccountName_, data_name,
                                                                    kCost_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const AccountType kAccountName_;
  const int32_t kCost_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class DataManagerPutResponseVisitor : public boost::static_visitor<> {
 public:
  DataManagerPutResponseVisitor(ServiceHandlerType* service, const PmidName& pmid_node,
                                nfs::MessageId message_id)
      : kService_(service), kPmidNode_(pmid_node), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutResponse<typename Name::data_type>(data_name, kPmidNode_,
                                                                    kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidNode_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType, typename RequestorIdType>
class GetRequestVisitor : public boost::static_visitor<> {
 public:
  GetRequestVisitor(ServiceHandlerType* service, RequestorIdType requestor_id,
                    nfs::MessageId message_id)
      : kService_(service),
        kRequestorId_(std::move(requestor_id)),
        kMessageId_(std::move(message_id)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleGet<typename Name::data_type, RequestorIdType>(
        data_name, kRequestorId_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const RequestorIdType kRequestorId_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class DataManagerSendDeleteVisitor : public boost::static_visitor<> {
 public:
  DataManagerSendDeleteVisitor(ServiceHandlerType* service, const uint64_t chunk_size,
                               const PmidName& pmid_node, nfs::MessageId message_id)
      : kService_(service), kChunkSize_(chunk_size),
        kPmidNode_(pmid_node), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    LOG(kWarning) << "DataManagerSendDeleteVisitor::operator() send delete request to node "
                  << HexSubstr(kPmidNode_->string()) << " for chunk "
                  << HexSubstr(data_name.value.string()) << " bearing message id "
                  << kMessageId_.data;
    kService_->template SendDeleteRequest<typename Name::data_type>(kPmidNode_, data_name,
                                                                    kChunkSize_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const uint64_t kChunkSize_;
  const PmidName kPmidNode_;
  nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PmidNodeGetVisitor : public boost::static_visitor<> {
 public:
  PmidNodeGetVisitor(ServiceHandlerType* service, const routing::SingleId& data_manager_node_id,
                     nfs::MessageId message_id)
      : kService_(service),
        kDataManagerNodeId_(data_manager_node_id.data),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleGet<typename Name::data_type>(data_name, kDataManagerNodeId_,
                                                            kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NodeId kDataManagerNodeId_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class DataManagerAccountQueryVisitor : public boost::static_visitor<> {
 public:
  DataManagerAccountQueryVisitor(ServiceHandlerType* service,
                                   const NodeId& sender_node_id)
      : kService_(service), kDataManagerNodeId_(sender_node_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandleAccountQuery(data_name, kDataManagerNodeId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NodeId kDataManagerNodeId_;
};

template <typename ServiceHandlerType>
class PmidNodeIntegrityCheckVisitor : public boost::static_visitor<> {
 public:
  PmidNodeIntegrityCheckVisitor(ServiceHandlerType* service, const NonEmptyString& random_string,
                                const routing::SingleSource& data_manager_node_id,
                                nfs::MessageId message_id)
      : kService_(service),
        kRandomString_(random_string),
        kDataManagerNodeId_(data_manager_node_id.data),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleIntegrityCheck<typename Name::data_type>(
        data_name, kRandomString_, kDataManagerNodeId_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString& kRandomString_;
  const NodeId kDataManagerNodeId_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PmidNodeDeleteVisitor : public boost::static_visitor<> {
 public:
  explicit PmidNodeDeleteVisitor(ServiceHandlerType* service) : kService_(service) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleDelete<typename Name::data_type>(data_name);
  }

 private:
  ServiceHandlerType* const kService_;
};

template <typename ServiceHandlerType>
class MaidManagerDeleteVisitor : public boost::static_visitor<> {
 public:
  MaidManagerDeleteVisitor(ServiceHandlerType* service, const MaidName& maid_name,
                           nfs::MessageId message_id)
      : kService_(service), kMaidName_(maid_name), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleDelete<typename Name::data_type>(kMaidName_, data_name, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidName_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class MaidManagerPutVersionRequestVisitor : public boost::static_visitor<> {
 public:
  MaidManagerPutVersionRequestVisitor(ServiceHandlerType* service, const MaidName& maid_name,
                                      StructuredDataVersions::VersionName old_version,
                                      StructuredDataVersions::VersionName new_version,
                                      nfs::MessageId message_id)
      : kService_(service),
        kMaidName_(maid_name),
        kOldVersion_(std::move(old_version)),
        kNewVersion_(std::move(new_version)),
        kMessageId_(message_id) {}

  template <typename DataNameType>
  void operator()(const DataNameType& data_name) {
    kService_->HandlePutVersionRequest(kMaidName_, data_name, kOldVersion_, kNewVersion_,
                                       kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidName_;
  const StructuredDataVersions::VersionName kOldVersion_, kNewVersion_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class MaidManagerDeleteBranchUntilForkVisitor : public boost::static_visitor<> {
 public:
  MaidManagerDeleteBranchUntilForkVisitor(ServiceHandlerType* service, const MaidName& maid_name,
                                          StructuredDataVersions::VersionName version,
                                          nfs::MessageId message_id)
      : kService_(service),
        kMaidName_(maid_name),
        kVersion_(std::move(version)),
        kMessageId_(message_id) {}

  template <typename DataNameType>
  void operator()(const DataNameType& data_name) {
    kService_->HandleDeleteBranchUntilFork(kMaidName_, data_name, kVersion_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidName_;
  const StructuredDataVersions::VersionName kVersion_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class MaidManagerCreateVersionTreeRequestVisitor : public boost::static_visitor<> {
 public:
  MaidManagerCreateVersionTreeRequestVisitor(ServiceHandlerType* service, const MaidName& maid_name,
                                             StructuredDataVersions::VersionName version,
                                             uint32_t max_versions, uint32_t max_branches,
                                             nfs::MessageId message_id)
      : kService_(service),
        kMaidName_(maid_name),
        kVersion_(std::move(version)),
        kMaxVersions_(max_versions),
        kMaxBranches_(max_branches),
        kMessageId_(message_id) {}

  template <typename DataNameType>
  void operator()(const DataNameType& data_name) {
    kService_->HandleCreateVersionTreeRequest(kMaidName_, data_name, kVersion_, kMaxVersions_,
                                              kMaxBranches_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidName_;
  const StructuredDataVersions::VersionName kVersion_;
  const uint32_t kMaxVersions_, kMaxBranches_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class MaidManagerCreateVersionTreeResponseVisitor : public boost::static_visitor<> {
 public:
  MaidManagerCreateVersionTreeResponseVisitor(ServiceHandlerType* service,
                                              const MaidName& maid_name,
                                              const maidsafe_error& return_code,
                                              nfs::MessageId message_id)
      : kService_(service),
        kMaidName_(maid_name),
        kReturnCode_(return_code),
        kMessageId_(message_id) {}

  template <typename DataNameType>
  void operator()(const DataNameType& data_name) {
    kService_->HandleCreateVersionTreeResponse(kMaidName_, data_name, kReturnCode_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidName_;
  const maidsafe_error kReturnCode_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class MaidManagerAccountRequestVisitor : public boost::static_visitor<> {
 public:
  MaidManagerAccountRequestVisitor(ServiceHandlerType* service,
                                   const NodeId& sender_node_id)
    : kService_(service), kMaidManagerNodeId_(sender_node_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandleAccountRequest(data_name, kMaidManagerNodeId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NodeId kMaidManagerNodeId_;
};

template <typename ServiceHandlerType>
class DataManagerDeleteVisitor : public boost::static_visitor<> {
 public:
  DataManagerDeleteVisitor(ServiceHandlerType* service, nfs::MessageId message_id)
      : kService_(service), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleDelete<typename Name::data_type>(data_name, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PmidManagerDeleteVisitor : public boost::static_visitor<> {
 public:
  PmidManagerDeleteVisitor(ServiceHandlerType* service, const PmidName& pmid_name,
                           int32_t size, nfs::MessageId message_id)
      : kService_(service), kPmidName_(pmid_name), kSize_(size), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleDelete<typename Name::data_type>(kPmidName_, data_name,
                                                               kSize_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const int32_t kSize_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PmidManagerPutResponseVisitor : public boost::static_visitor<> {
 public:
  PmidManagerPutResponseVisitor(ServiceHandlerType* service, const PmidName& pmid_name,
                                nfs::MessageId message_id)
      : kService_(service), kPmidName_(pmid_name), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template SendPutResponse<typename Name::data_type>(data_name, kPmidName_,
                                                                  kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PmidManagerPutResponseFailureVisitor : public boost::static_visitor<> {
 public:
  PmidManagerPutResponseFailureVisitor(ServiceHandlerType* service, const PmidName& pmid_name,
                                       const uint64_t size,
                                       const int64_t available_size,
                                       const maidsafe_error& return_code, nfs::MessageId message_id)
      : kService_(service),
        kPmidName_(pmid_name),
        kSize_(size),
        kAvailableSize_(available_size),
        kReturnCode_(return_code),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutFailure<typename Name::data_type>(
        data_name, kPmidName_, kSize_, kAvailableSize_, kReturnCode_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const uint64_t kSize_;
  const int64_t kAvailableSize_;
  const maidsafe_error kReturnCode_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PmidManagerFalseNotificationVisitor : public boost::static_visitor<> {
 public:
  PmidManagerFalseNotificationVisitor(ServiceHandlerType* service, const PmidName& pmid_name,
                                      int32_t size, nfs::MessageId message_id)
      : kService_(service), kPmidName_(pmid_name), kSize_(size), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleFalseNotification<typename Name::data_type>(data_name, kPmidName_,
                                                                          kSize_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const int32_t kSize_;
  const nfs::MessageId kMessageId_;
};

// ==================================== VersionHandler Visitors=====================================

template <typename SourcePersonaType>
class VersionHandlerGetVisitor : public boost::static_visitor<> {
 public:
  VersionHandlerGetVisitor(VersionHandlerService* service, const Identity& originator,
                           nfs::MessageId message_id)
      : kService_(service),
        kRequestor_(NodeId(std::move(originator.string()))),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandleGetVersions(Key(data_name, Name::data_type::Tag::kValue), kRequestor_,
                                 kMessageId_);
  }

 private:
  VersionHandlerService* const kService_;
  detail::Requestor<SourcePersonaType> kRequestor_;
  nfs::MessageId kMessageId_;
};

template <typename SourcePersonaType>
class VersionHandlerGetBranchVisitor : public boost::static_visitor<> {
 public:
  VersionHandlerGetBranchVisitor(VersionHandlerService* service,
                                 const StructuredDataVersions::VersionName version_name,
                                 const Identity& originator, nfs::MessageId message_id)
      : kService_(service),
        kVersionName_(std::move(version_name)),
        kRequestor_(NodeId(std::move(originator.string()))),
        kMessageId_(std::move(message_id)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandleGetBranch(Key(data_name, Name::data_type::Tag::kValue), kVersionName_,
                               kRequestor_, kMessageId_);
  }

 private:
  VersionHandlerService* const kService_;
  StructuredDataVersions::VersionName kVersionName_;
  detail::Requestor<SourcePersonaType> kRequestor_;
  nfs::MessageId kMessageId_;
};

class VersionHandlerPutVisitor : public boost::static_visitor<> {
 public:
  VersionHandlerPutVisitor(VersionHandlerService* service,
                           const StructuredDataVersions::VersionName& old_version,
                           const StructuredDataVersions::VersionName& new_version,
                           const Identity& originator, nfs::MessageId message_id)
      : kService_(service),
        kOldVersion_(old_version),
        kNewVersion_(new_version),
        kMessageId_(message_id),
        kOriginator_(std::move(originator)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandlePutVersion(Key(data_name, Name::data_type::Tag::kValue), kOldVersion_,
                                kNewVersion_, kOriginator_, kMessageId_);
  }

 private:
  VersionHandlerService* const kService_;
  StructuredDataVersions::VersionName kOldVersion_, kNewVersion_;
  nfs::MessageId kMessageId_;
  const Identity kOriginator_;
};

class VersionHandlerDeleteBranchVisitor : public boost::static_visitor<> {
 public:
  VersionHandlerDeleteBranchVisitor(VersionHandlerService* service,
                                    const StructuredDataVersions::VersionName& version_name,
                                    const Identity& originator)
      : kService_(service), kVersionName_(version_name), kOriginator_(std::move(originator)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandleDeleteBranchUntilFork(Key(data_name, Name::data_type::Tag::kValue),
                                           kVersionName_, kOriginator_);
  }

 private:
  VersionHandlerService* const kService_;
  const StructuredDataVersions::VersionName kVersionName_;
  const Identity kOriginator_;
};

class VersionHandlerCreateVersionTreeVisitor : public boost::static_visitor<> {
 public:
  VersionHandlerCreateVersionTreeVisitor(VersionHandlerService* service,
                                         const StructuredDataVersions::VersionName& version,
                                         const Identity& originator, uint32_t max_versions,
                                         uint32_t max_branches, nfs::MessageId message_id)
      : kService_(service),
        kVersion_(version),
        kMaxVersions_(max_versions),
        kMaxBranches_(max_branches),
        kMessageId_(message_id),
        kOriginator_(Identity(originator)) {}

  template <typename DataNameType>
  void operator()(const DataNameType& data_name) {
    kService_->HandleCreateVersionTree(Key(data_name, DataNameType::data_type::Tag::kValue),
                                       kVersion_, kOriginator_, kMaxVersions_, kMaxBranches_,
                                       kMessageId_);
  }

 private:
  VersionHandlerService* const kService_;
  const StructuredDataVersions::VersionName kVersion_;
  const uint32_t kMaxVersions_, kMaxBranches_;
  const nfs::MessageId kMessageId_;
  const Identity kOriginator_;
};

class CheckDataNameVisitor : public boost::static_visitor<bool> {
 public:
  explicit CheckDataNameVisitor(const NonEmptyString& content) : kContent_(content) {}

  template <typename DataNameType>
  result_type operator()(const DataNameType& data_name) {
    try {
      return (typename DataNameType::data_type(
                  data_name, typename DataNameType::data_type::serialised_type(kContent_)).name() ==
              data_name);
    }
    catch (const maidsafe_error& error) {
      LOG(kWarning) << "Failed to deserialise data" << error.code();
      return false;
    }
  }

 private:
  NonEmptyString kContent_;
};

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/utils-inl.h"

#endif  // MAIDSAFE_VAULT_OPERATION_VISITORS_H_
