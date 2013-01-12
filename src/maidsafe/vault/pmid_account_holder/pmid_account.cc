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

#include "maidsafe/vault/pmid_account_holder/pmid_account.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account_pb.h"
#include "maidsafe/vault/maid_account_holder/maid_account_pb.h"
#include "maidsafe/common/log.h"

namespace maidsafe {

namespace vault {

void PmidRecord::Parse(const NonEmptyString& serialised_pmidsize) {
  protobuf::PmidRecord proto_pmid_record;
  if (!proto_pmid_record.ParseFromString(serialised_pmidsize.string()) ||
      !proto_pmid_record.IsInitialized()) {
    LOG(kError) << "Failed to parse pmid_size.";
    ThrowError(NfsErrors::pmid_size_parsing_error);
  }

  pmid_name = Identity(proto_pmid_record.pmid_name());
  stored_count = proto_pmid_record.stored_count();
  stored_total_size = proto_pmid_record.stored_total_size();
  lost_count = proto_pmid_record.lost_count();
  lost_total_size = proto_pmid_record.lost_total_size();
}

NonEmptyString PmidRecord::Serialise() {
  protobuf::PmidRecord proto_pmid_record;
  proto_pmid_record.set_pmid_name(pmid_name.string());
  proto_pmid_record.set_stored_count(stored_count);
  proto_pmid_record.set_stored_total_size(stored_total_size);
  proto_pmid_record.set_lost_count(lost_count);
  proto_pmid_record.set_lost_total_size(lost_total_size);
  return NonEmptyString(proto_pmid_record.SerializeAsString());
}

NonEmptyString PmidTotal::Serialise() {
  /*protobuf::PmidRecord proto_pmid_record;
  proto_pmid_record.ParseFromString(pmid_record.Serialise().string());
  nfs::protobuf::PmidRegistration proto_pmid_registration;
  proto_pmid_registration.ParseFromString(registration.Serialise().string());

  protobuf::PmidTotals proto_pmid_total;
  *(proto_pmid_total.mutable_pmid_record()) = proto_pmid_record;
  *(proto_pmid_total.mutable_serialised_pmid_registration()) = proto_pmid_registration;
  return NonEmptyString(proto_pmid_total.SerializeAsString());*/
  return NonEmptyString();
}

NonEmptyString DataElement::Serialise() {
  protobuf::DataElements proto_data_element;
  proto_data_element.set_name(name_.string());
  proto_data_element.set_size(size);
  return NonEmptyString(proto_data_element.SerializeAsString());
}

}  // namespace vault

}  // namespace maidsafe
