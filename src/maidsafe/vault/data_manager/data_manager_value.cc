/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/data_manager/metadata_value.h"

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

MetadataValue::serialised_type MetadataValue::Serialise() const {
//  if (!subscribers || ((*subscribers) == 0) || (online_pmid_name.empty()))
//    ThrowError(CommonErrors::uninitialised);  // Cannot serialise if not a complete db value
// FIXME
  protobuf::MetadataValue metadata_value_proto;
  metadata_value_proto.set_size(data_size);
  metadata_value_proto.set_subscribers(*subscribers);
  for (const auto& i: online_pmid_name)
    metadata_value_proto.add_online_pmid_name(i->string());
  for (const auto& i: offline_pmid_name)
    metadata_value_proto.add_offline_pmid_name(i->string());
  assert(metadata_value_proto.IsInitialized());
  return serialised_type(NonEmptyString(metadata_value_proto.SerializeAsString()));
}

Metadata::Metadata(const DataNameVariant& data_name, ManagerDb<DataManager>* metadata_db,
                   int32_t data_size)
    : data_name_(data_name),
      value_([&metadata_db, data_name, data_size, this]()->MetadataValue {
                assert(metadata_db);
                try {
                  return metadata_db->Get(DbKey(data_name));
                }
                catch(const std::exception& /*ex*/) {
                  return MetadataValue(data_size);
                }
             } ()),
      strong_guarantee_(on_scope_exit::ExitAction()) {
  strong_guarantee_.SetAction(on_scope_exit::RevertValue(value_));
}

Metadata::Metadata(const DataNameVariant& data_name, ManagerDb<DataManager>* metadata_db)
  : data_name_(data_name),
    value_([&metadata_db, data_name, this]()->MetadataValue {
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
