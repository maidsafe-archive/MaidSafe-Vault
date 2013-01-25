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

#include "maidsafe/vault/pmid_record.h"

#include "maidsafe/common/log.h"

#include "maidsafe/vault/pmid_account_pb.h"


namespace maidsafe {

namespace vault {

PmidRecord::PmidRecord()
    : pmid_name(),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0) {}

PmidRecord::PmidRecord(const PmidName& pmid_name_in)
    : pmid_name(pmid_name_in),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0) {}

PmidRecord::PmidRecord(const protobuf::PmidRecord& proto_pmid_record)
    : pmid_name(),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0) {
  if (!proto_pmid_record.IsInitialized()) {
    LOG(kError) << "Failed to construct pmid_record.";
    ThrowError(CommonErrors::invalid_parameter);
  }
  pmid_name = PmidName(Identity(proto_pmid_record.pmid_name()));
  stored_count = proto_pmid_record.stored_count();
  stored_total_size = proto_pmid_record.stored_total_size();
  lost_count = proto_pmid_record.lost_count();
  lost_total_size = proto_pmid_record.lost_total_size();
}

protobuf::PmidRecord PmidRecord::ToProtobuf() const {
  protobuf::PmidRecord proto_pmid_record;
  proto_pmid_record.set_pmid_name(pmid_name->string());
  proto_pmid_record.set_stored_count(stored_count);
  proto_pmid_record.set_stored_total_size(stored_total_size);
  proto_pmid_record.set_lost_count(lost_count);
  proto_pmid_record.set_lost_total_size(lost_total_size);
  return proto_pmid_record;
}

PmidRecord::PmidRecord(const PmidRecord& other)
    : pmid_name(other.pmid_name),
      stored_count(other.stored_count),
      stored_total_size(other.stored_total_size),
      lost_count(other.lost_count),
      lost_total_size(other.lost_total_size) {}

PmidRecord& PmidRecord::operator=(const PmidRecord& other) {
  pmid_name = other.pmid_name;
  stored_count = other.stored_count;
  stored_total_size = other.stored_total_size;
  lost_count = other.lost_count;
  lost_total_size = other.lost_total_size;
  return *this;
}

PmidRecord::PmidRecord(PmidRecord&& other)
    : pmid_name(std::move(other.pmid_name)),
      stored_count(std::move(other.stored_count)),
      stored_total_size(std::move(other.stored_total_size)),
      lost_count(std::move(other.lost_count)),
      lost_total_size(std::move(other.lost_total_size)) {}

PmidRecord& PmidRecord::operator=(PmidRecord&& other) {
  pmid_name = std::move(other.pmid_name);
  stored_count = std::move(other.stored_count);
  stored_total_size = std::move(other.stored_total_size);
  lost_count = std::move(other.lost_count);
  lost_total_size = std::move(other.lost_total_size);
  return *this;
}

}  // namespace vault

}  // namespace maidsafe
