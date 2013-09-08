/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/data_manager/metadata.h"

#include <string>

#include "maidsafe/vault/utils.h"



namespace maidsafe {

namespace vault {

Metadata::Metadata(const DataNameVariant& data_name, ManagerDb<DataManager>* metadata_db,
                   int32_t data_size)
    : data_name_(data_name),
      value_([&metadata_db, data_name, data_size, this]()->DataManagerValue {
                assert(metadata_db);
                try {
                  return metadata_db->Get(DbKey(data_name));
                }
                catch(const std::exception& /*ex*/) {
                  return DataManagerValue(data_size);
                }
             } ()),
      strong_guarantee_(on_scope_exit::ExitAction()) {
  strong_guarantee_.SetAction(on_scope_exit::RevertValue(value_));
}

Metadata::Metadata(const DataNameVariant& data_name, ManagerDb<DataManager>* metadata_db)
  : data_name_(data_name),
    value_([&metadata_db, data_name, this]()->DataManagerValue {
            assert(metadata_db);
            return metadata_db->Get(DbKey(data_name));
          } ()),
    strong_guarantee_(on_scope_exit::ExitAction()) {
  strong_guarantee_.SetAction(on_scope_exit::RevertValue(value_));
}

void Metadata::SaveChanges(ManagerDb<DataManager>* metadata_db) {
  assert(metadata_db);
  //TODO(Prakash): Handle case of modifying unique data
  if (*value_.subscribers < 1) {
    metadata_db->Delete(DbKey(data_name_));
  } else {
    auto kv_pair(std::make_pair(DbKey(data_name_), value_));
    metadata_db->Put(kv_pair);
  }
  strong_guarantee_.Release();
}

}  // namespace vault

}  // namespace maidsafe
