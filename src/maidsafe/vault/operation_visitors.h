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

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/vault/version_handler/key.h"
#include"maidsafe/vault/version_handler/service.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

class VersionHandlerService;

namespace detail {

template <typename ServiceHandlerType>
class MaidManagerPutVisitor : public boost::static_visitor<> {
 public:
  MaidManagerPutVisitor(ServiceHandlerType* service, NonEmptyString content, NodeId sender,
                        Identity pmid_hint, nfs::MessageId message_id)
      : kService_(service),
        kContent_(std::move(content)),
        kSender_(std::move(sender)),
        kPmidHint_(std::move(pmid_hint)),
        kMessageId_(std::move(message_id)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    LOG(kVerbose) << "MaidManagerPutVisitor HandlePut";
    kService_->HandlePut(MaidName(Identity(kSender_.string())),
                                  typename Name::data_type(data_name,
                                      typename Name::data_type::serialised_type(kContent_)),
                                  PmidName(kPmidHint_), kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString kContent_;
  const NodeId kSender_;
  const Identity kPmidHint_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class DataManagerPutVisitor : public boost::static_visitor<> {
 public:
  DataManagerPutVisitor(ServiceHandlerType* service, NonEmptyString content, Identity maid_name,
                        Identity pmid_name, nfs::MessageId message_id)
      : kService_(service),
        kContent_(std::move(content)),
        kMaidName_(std::move(maid_name)),
        kPmidName_(std::move(pmid_name)),
        kMessageId_(std::move(message_id)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandlePut(
        typename Name::data_type(data_name, typename Name::data_type::serialised_type(kContent_)),
        kMaidName_, kPmidName_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString kContent_;
  const MaidName kMaidName_;
  const PmidName kPmidName_;
  const nfs::MessageId kMessageId_;
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
        kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString kContent_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PutResponseFailureVisitor : public boost::static_visitor<> {
 public:
  PutResponseFailureVisitor(ServiceHandlerType* service, const Identity& pmid_node,
                            const maidsafe_error& return_code, nfs::MessageId message_id)
      : kService_(service),
        kPmidNode_(pmid_node),
        kReturnCode_(return_code),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutFailure<typename Name::data_type>(
        data_name, kPmidNode_, kMessageId_, maidsafe_error(kReturnCode_));
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidNode_;
  const maidsafe_error kReturnCode_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class MaidManagerPutResponseFailureVisitor : public boost::static_visitor<> {
 public:
  MaidManagerPutResponseFailureVisitor(ServiceHandlerType* service, const MaidName& maid_node,
                                       const maidsafe_error& return_code,
                                       nfs::MessageId message_id)
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

template <typename ServiceHandlerType>
class MaidManagerPutResponseVisitor : public boost::static_visitor<> {
 public:
  MaidManagerPutResponseVisitor(ServiceHandlerType* service, const Identity& maid_node,
                                int32_t cost, nfs::MessageId message_id)
      : kService_(service), kMaidNode_(maid_node), kCost_(cost), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutResponse<typename Name::data_type>(kMaidNode_, data_name,
                                                                    kCost_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidNode_;
  const int32_t kCost_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class DataManagerPutResponseVisitor : public boost::static_visitor<> {
 public:
  DataManagerPutResponseVisitor(ServiceHandlerType* service, const PmidName& pmid_node,
                                int32_t size, nfs::MessageId message_id)
      : kService_(service), kPmidNode_(pmid_node), kSize_(size), kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutResponse<typename Name::data_type>(data_name, kPmidNode_,
                                                                    kSize_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidNode_;
  const int32_t kSize_;
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

template<typename ServiceHandlerType>
class DataManagerSendDeleteVisitor : public boost::static_visitor<> {
 public:
  DataManagerSendDeleteVisitor(ServiceHandlerType* service, const PmidName& pmid_node,
                               nfs::MessageId message_id)
      : kService_(service), kPmidNode_(pmid_node), kMessageId_(message_id) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    LOG(kWarning) << "DataManagerSendDeleteVisitor::operator() send delete request to node "
                  << HexSubstr(kPmidNode_->string()) << " for chunk "
                  << HexSubstr(data_name.value.string())
                  << " bearing message id " << kMessageId_.data;
    kService_->template SendDeleteRequest<typename Name::data_type>(kPmidNode_, data_name,
                                                                    kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidNode_;
  nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class PmidNodeGetVisitor : public boost::static_visitor<> {
 public:
  PmidNodeGetVisitor(ServiceHandlerType* service, const routing::SingleId& data_manager_node_id,
                     nfs::MessageId message_id)
      : kService_(service), kDataManagerNodeId_(data_manager_node_id.data),
        kMessageId_(message_id) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleGet<typename Name::data_type>(data_name, kDataManagerNodeId_,
                                                            kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NodeId kDataManagerNodeId_;
  const nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class PmidNodeIntegrityCheckVisitor : public boost::static_visitor<> {
 public:
  PmidNodeIntegrityCheckVisitor(ServiceHandlerType* service,
                                const NonEmptyString& random_string,
                                const routing::SingleSource& data_manager_node_id,
                                nfs::MessageId message_id)
      : kService_(service),
        kRandomString_(random_string),
        kDataManagerNodeId_(data_manager_node_id.data),
        kMessageId_(message_id) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleIntegrityCheck<typename Name::data_type>(data_name, kRandomString_,
                                                                       kDataManagerNodeId_,
                                                                       kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const NonEmptyString& kRandomString_;
  const NodeId kDataManagerNodeId_;
  const nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class PmidNodeDeleteVisitor : public boost::static_visitor<> {
 public:
  PmidNodeDeleteVisitor(ServiceHandlerType* service) : kService_(service) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleDelete<typename Name::data_type>(data_name);
  }

 private:
  ServiceHandlerType* const kService_;
};

template<typename ServiceHandlerType>
class MaidManagerDeleteVisitor : public boost::static_visitor<> {
 public:
  MaidManagerDeleteVisitor(ServiceHandlerType* service, const MaidName& maid_name,
                           nfs::MessageId message_id)
      : kService_(service), kMaidName_(maid_name), kMessageId_(message_id) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleDelete<typename Name::data_type>(kMaidName_, data_name, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidName_;
  const nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class MaidManagerPutVersionVisitor : public boost::static_visitor<> {
 public:
  MaidManagerPutVersionVisitor(ServiceHandlerType* service, const MaidName& maid_name,
                               StructuredDataVersions::VersionName old_version,
                               StructuredDataVersions::VersionName new_version,
                               nfs::MessageId message_id)
      : kService_(service), kMaidName_(maid_name), kOldVersion_(std::move(old_version)),
        kNewVersion_(std::move(new_version)), kMessageId_(message_id) {}

  template<typename DataNameType>
  void operator()(const DataNameType& data_name) {
    kService_->HandlePutVersion(kMaidName_, data_name, kOldVersion_, kNewVersion_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidName_;
  const StructuredDataVersions::VersionName kOldVersion_, kNewVersion_;
  const nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class MaidManagerDeleteBranchUntilForkVisitor : public boost::static_visitor<> {
 public:
  MaidManagerDeleteBranchUntilForkVisitor(ServiceHandlerType* service, const MaidName& maid_name,
                                          StructuredDataVersions::VersionName version,
                                          nfs::MessageId message_id)
      : kService_(service), kMaidName_(maid_name), kVersion_(std::move(version)),
        kMessageId_(message_id) {}

  template<typename DataNameType>
  void operator()(const DataNameType& data_name) {
    kService_->HandleDeleteBranchUntilFork(kMaidName_, data_name, kVersion_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const MaidName kMaidName_;
  const StructuredDataVersions::VersionName kVersion_;
  const nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class DataManagerDeleteVisitor : public boost::static_visitor<> {
 public:
  DataManagerDeleteVisitor(ServiceHandlerType* service, nfs::MessageId message_id)
      : kService_(service), kMessageId_(message_id) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleDelete<typename Name::data_type>(data_name, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class PmidManagerDeleteVisitor : public boost::static_visitor<> {
 public:
  PmidManagerDeleteVisitor(ServiceHandlerType* service, const PmidName& pmid_name,
                           nfs::MessageId message_id)
      : kService_(service), kPmidName_(pmid_name), kMessageId_(message_id) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleDelete<typename Name::data_type>(kPmidName_, data_name, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class PmidManagerPutResponseVisitor : public boost::static_visitor<> {
 public:
  PmidManagerPutResponseVisitor(ServiceHandlerType* service, int32_t size,
                                const PmidName& pmid_name, nfs::MessageId message_id)
      : kService_(service), kPmidName_(pmid_name), kSize_(size), kMessageId_(message_id) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    kService_->template SendPutResponse<typename Name::data_type>(data_name, kSize_, kPmidName_,
                                                                  kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const int32_t kSize_;
  const nfs::MessageId kMessageId_;
};

template <typename ServiceHandlerType>
class PmidManagerPutResponseFailureVisitor : public boost::static_visitor<> {
 public:
  PmidManagerPutResponseFailureVisitor(ServiceHandlerType* service, const PmidName& pmid_name,
                                       const int64_t available_size,
                                       const maidsafe_error& return_code,
                                       nfs::MessageId message_id)
      : kService_(service),
        kPmidName_(pmid_name),
        kAvailableSize_(available_size),
        kReturnCode_(return_code),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandlePutFailure<typename Name::data_type>(data_name, kPmidName_,
                                                                   kAvailableSize_,
                                                                   kReturnCode_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const int64_t kAvailableSize_;
  const maidsafe_error kReturnCode_;
  const nfs::MessageId kMessageId_;
};

template<typename ServiceHandlerType>
class PmidManagerFalseNotificationVisitor : public boost::static_visitor<> {
 public:
  PmidManagerFalseNotificationVisitor(ServiceHandlerType* service, const PmidName& pmid_name,
                                      nfs::MessageId message_id)
      : kService_(service), kPmidName_(pmid_name), kMessageId_(message_id) {}

  template<typename Name>
  void operator()(const Name& data_name) {
    kService_->template HandleFalseNotification<typename Name::data_type>(
        data_name, kPmidName_, kMessageId_);
  }

 private:
  ServiceHandlerType* const kService_;
  const PmidName kPmidName_;
  const nfs::MessageId kMessageId_;
};

// ==================================== VersionHandler Visitors=====================================

template <typename SourcePersonaType>
class VersionHandlerGetVisitor : public boost::static_visitor<> {
 public:
  VersionHandlerGetVisitor(VersionHandlerService* service, Identity originator,
                           nfs::MessageId message_id)
      : kService_(service), kRequestor_(NodeId(std::move(originator.string()))),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandleGetVersions(
        VersionHandlerKey(data_name, Name::data_type::Tag::kValue,
                          Identity(kRequestor_.node_id.string())),
        kRequestor_, kMessageId_);
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
                                 Identity originator, nfs::MessageId message_id)
      : kService_(service), kVersionName_(std::move(version_name)),
        kRequestor_(NodeId(std::move(originator.string()))), kMessageId_(std::move(message_id)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandleGetBranch(
        VersionHandlerKey(data_name, Name::data_type::Tag::kValue,
                          Identity(kRequestor_.node_id.string())),
        kVersionName_, kRequestor_, kMessageId_);
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
                           StructuredDataVersions::VersionName old_version,
                           StructuredDataVersions::VersionName new_version, NodeId sender,
                           nfs::MessageId message_id)
      : kService_(service), kOldVersion_(std::move(old_version)),
        kNewVersion_(std::move(new_version)), kSender_(std::move(sender)),
        kMessageId_(message_id) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandlePutVersion(VersionHandlerKey(data_name, Name::data_type::Tag::kValue,
                          Identity(kSender_.string())),
        kOldVersion_, kNewVersion_, kSender_, kMessageId_);
  }

 private:
  VersionHandlerService* const kService_;
  StructuredDataVersions::VersionName kOldVersion_, kNewVersion_;
  NodeId kSender_;
  nfs::MessageId kMessageId_;
};

class VersionHandlerDeleteBranchVisitor : public boost::static_visitor<> {
 public:
  VersionHandlerDeleteBranchVisitor(VersionHandlerService* service,
                                    const StructuredDataVersions::VersionName version_name,
                                    NodeId sender)
      : kService_(service), kVersionName_(std::move(version_name)),
        kSender_(std::move(sender)) {}

  template <typename Name>
  void operator()(const Name& data_name) {
    kService_->HandleDeleteBranchUntilFork(
        VersionHandlerKey(data_name, Name::data_type::Tag::kValue, Identity(kSender_.string())),
        kVersionName_, kSender_);
  }

 private:
  VersionHandlerService* const kService_;
  StructuredDataVersions::VersionName kVersionName_;
  NodeId kSender_;
};

class CheckDataNameVisitor : public boost::static_visitor<bool> {
 public:
  CheckDataNameVisitor(const NonEmptyString& content)
      : kContent_(content) {}

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
