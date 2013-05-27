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

#include "maidsafe/vault/metadata_manager/metadata.h"

#include <string>

#include "maidsafe/vault/utils.h"



namespace maidsafe {

namespace vault {

MetadataValue::MetadataValue(const serialised_type& serialised_metadata_value)
  : data_size(),
    subscribers(),
    online_pmid_name(),
    offline_pmid_name() {
  protobuf::MetadataValue metadata_value_proto;
  if (!metadata_value_proto.ParseFromString(serialised_metadata_value->string()) ||
      metadata_value_proto.size() != 0 ||
      metadata_value_proto.subscribers() < 1) {
    LOG(kError) << "Failed to read or parse serialised metadata value";
    ThrowError(CommonErrors::parsing_error);
  } else {
    data_size = metadata_value_proto.size();
    subscribers = metadata_value_proto.subscribers();
    for (auto& i : metadata_value_proto.online_pmid_name())
      online_pmid_name.insert(PmidName(Identity(i)));
    for (auto& i : metadata_value_proto.offline_pmid_name())
      offline_pmid_name.insert(PmidName(Identity(i)));
  }
}

MetadataValue::MetadataValue(int size)
    : data_size(size),
      subscribers(0),
      online_pmid_name(),
      offline_pmid_name() {
  if (size < 1)
    ThrowError(CommonErrors::invalid_parameter);
}

MetadataValue::serialised_type MetadataValue::Serialise() {
  protobuf::MetadataValue metadata_value_proto;
  metadata_value_proto.set_size(data_size);
  metadata_value_proto.set_subscribers(subscribers);
  for (const auto& i: online_pmid_name)
    metadata_value_proto.add_online_pmid_name(i->string());
  for (const auto& i: offline_pmid_name)
    metadata_value_proto.add_offline_pmid_name(i->string());
  assert(metadata_value_proto.IsInitialized());
  return serialised_type(NonEmptyString(metadata_value_proto.SerializeAsString()));
}

Metadata::Metadata(const DataNameVariant& data_name, MetadataDb* metadata_db,
                         int32_t data_size)
    : data_name_(data_name),
      value_([&metadata_db, data_name, data_size, this]()->MetadataValue {
              assert(metadata_db);
              auto metadata_value_string(metadata_db->Get(data_name));
              if (metadata_value_string.string().empty()) {
                return MetadataValue(data_size);
              }
              return MetadataValue(MetadataValue::serialised_type(metadata_value_string));
              } ()),
      strong_guarantee_(on_scope_exit::ExitAction()) {
  strong_guarantee_.SetAction(on_scope_exit::RevertValue(value_));
}

Metadata::Metadata(const DataNameVariant& data_name, MetadataDb* metadata_db)
  : data_name_(data_name),
    value_([&metadata_db, data_name, this]()->MetadataValue {
            assert(metadata_db);
            auto metadata_value_string(metadata_db->Get(data_name));
            if (metadata_value_string.string().empty()) {
              LOG(kError) << "Failed to find metadata entry";
              ThrowError(CommonErrors::no_such_element);
            }
            return MetadataValue(MetadataValue::serialised_type(metadata_value_string));
          } ()),
    strong_guarantee_(on_scope_exit::ExitAction()) {
  strong_guarantee_.SetAction(on_scope_exit::RevertValue(value_));
}

void Metadata::SaveChanges(MetadataDb* metadata_db) {
  assert(metadata_db);
  //TODO(Prakash): Handle case of modifying unique data
  if (value_.subscribers < 1) {
    metadata_db->Delete(data_name_);
  } else {
    auto kv_pair(std::make_pair(data_name_, value_.Serialise()));
    metadata_db->Put(kv_pair);
  }
  strong_guarantee_.Release();
}

}  // namespace vault

}  // namespace maidsafe
