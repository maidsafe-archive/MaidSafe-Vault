/***************************************************************************************************
 *  Copyright 2012 maidsafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use. The use of this code is governed by the licence file licence.txt found in the root of     *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit written *
 *  permission of the board of directors of MaidSafe.net.                                          *
 **************************************************************************************************/

#include <string>

#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/pmid_account_holder/pmid_account.h"


namespace maidsafe {

namespace vault {

namespace {

PmidRecord ParsePmidRecord(const PmidAccount::serialised_type& serialised_pmid_account) {
  protobuf::PmidAccount pmid_account;
  if (!pmid_account.ParseFromString(serialised_pmid_account.data.string()))
    ThrowError(CommonErrors::parsing_error);
  return PmidRecord(pmid_account.pmid_record());
}

}  // namespace

protobuf::DataElement PmidAccount::DataElement::ToProtobuf() const {
  GetTagValueAndIdentityVisitor type_and_name_visitor;
  auto type_and_name(boost::apply_visitor(type_and_name_visitor, data_name_variant));
  protobuf::DataElement data_element;
  data_element.set_name(type_and_name.second.string());
  data_element.set_type(static_cast<int32_t>(type_and_name.first));
  data_element.set_size(size);
  return data_element;
}

PmidAccount::PmidAccount(const PmidName& pmid_name,  Db& db, const NodeId& this_node_id)
    : pmid_name_(pmid_name),
      this_node_id_(this_node_id),
      pmid_record_(pmid_name),
      data_holder_status_(DataHolderStatus::kUp),
      account_db_(new AccountDb(db)),
      sync_(account_db_.get(), this_node_id) {}

PmidAccount::PmidAccount(const PmidName& pmid_name,
                         Db& db,
                         const NodeId& this_node_id,
                         const NodeId& /*source_id*/,
                         const serialised_type& serialised_pmid_account_details)
    : pmid_name_(pmid_name),
      this_node_id_(this_node_id),
      pmid_record_(ParsePmidRecord(serialised_pmid_account_details)),
      data_holder_status_(DataHolderStatus::kUp),
      account_db_(new AccountDb(db)),
      sync_(account_db_.get(), this_node_id) {
  protobuf::PmidAccount pmid_account;
  if (!pmid_account.ParseFromString(serialised_pmid_account_details->string())) {
    LOG(kError) << "Failed to parse pmid_account.";
    ThrowError(CommonErrors::parsing_error);
  }
  //for (auto& recent_data : pmid_account.recent_data_stored()) {
  //  DataElement data_element(GetDataNameVariant(static_cast<DataTagValue>(recent_data.type()),
  //                                              Identity(recent_data.name())),
  //                           recent_data.size());
  //  recent_data_stored_.push_back(data_element);
  //}
}

PmidAccount::serialised_type PmidAccount::Serialise() {
    protobuf::PmidAccount pmid_account;
  *(pmid_account.mutable_pmid_record()) = pmid_record_.ToProtobuf();
  auto unresolved_data(sync_.GetUnresolvedData());
  for (const auto& unresolved_entry : unresolved_data)
    pmid_account.add_serialised_unresolved_entry(unresolved_entry.Serialise()->string());
  return serialised_type(NonEmptyString(pmid_account.SerializeAsString()));
}


}  // namespace vault

}  // namespace maidsafe
