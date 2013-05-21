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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_INL_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_INL_H_

#include <set>
#include <string>
#include <vector>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/error.h"
#include "maidsafe/common/utils.h"

namespace maidsafe {

namespace vault {


template<typename Data>
Metadata<Data>::Metadata(const typename Data::name_type& data_name, MetadataDb* metadata_db,
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

template<typename Data>
Metadata<Data>::Metadata(const typename Data::name_type& data_name, MetadataDb* metadata_db)
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

template<typename Data>
void Metadata<Data>::SaveChanges(MetadataDb* metadata_db) {
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

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_INL_H_
