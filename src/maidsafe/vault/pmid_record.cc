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

#include <string>

#include "maidsafe/common/log.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account_pb.h"
#include "maidsafe/vault/maid_account_holder/maid_account_pb.h"

namespace maidsafe {

namespace vault {

PmidRecord::PmidRecord(const PmidName& pmid_name)
    : kPmidName(pmid_name),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0) {}

PmidRecord::PmidRecord(const NonEmptyString& serialised_pmid_record)
    : kPmidName([&serialised_pmid_record]()->PmidName {
                    protobuf::PmidRecord proto_pmid_record;
                    proto_pmid_record.ParseFromString(serialised_pmid_record.string());
                    return PmidName(Identity(proto_pmid_record.pmid_name()));
                }()),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0) {
  protobuf::PmidRecord proto_pmid_record;
  if (!proto_pmid_record.ParseFromString(serialised_pmid_record.string())) {
    LOG(kError) << "Failed to parse pmid_size.";
    ThrowError(NfsErrors::pmid_size_parsing_error);
  }
  stored_count = proto_pmid_record.stored_count();
  stored_total_size = proto_pmid_record.stored_total_size();
  lost_count = proto_pmid_record.lost_count();
  lost_total_size = proto_pmid_record.lost_total_size();
}

NonEmptyString PmidRecord::Serialise() const {
  protobuf::PmidRecord proto_pmid_record;
  proto_pmid_record.set_pmid_name(kPmidName->string());
  proto_pmid_record.set_stored_count(stored_count);
  proto_pmid_record.set_stored_total_size(stored_total_size);
  proto_pmid_record.set_lost_count(lost_count);
  proto_pmid_record.set_lost_total_size(lost_total_size);
  return NonEmptyString(proto_pmid_record.SerializeAsString());
}

}  // namespace vault

}  // namespace maidsafe
