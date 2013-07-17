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

#include "maidsafe/vault/data_manager/data_manager_value.h"

#include <string>

//#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace delete_me {

DataManagerValue::DataManagerValue(const serialised_type& serialised_metadata_value)
  : data_size(),
    subscribers(),
    online_pmid_name(),
    offline_pmid_name() {
  protobuf::DataManagerValue metadata_value_proto;
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

DataManagerValue::DataManagerValue(const PmidName& pmid_name, int size)
    : data_size(size),
      subscribers(0),
      online_pmid_name(),
      offline_pmid_name() {
  if (size < 1)
    ThrowError(CommonErrors::invalid_parameter);
  online_pmid_name.insert(pmid_name);
}

void DataManagerValue::AddPmid(const PmidName& /*pmid_name*/) {}
void DataManagerValue::RemovePmid(const PmidName& /*pmid_name*/) {}
void DataManagerValue::Increamentsubscribers() {}
void DataManagerValue::Decreamentsubscribers() {}
void DataManagerValue::SetPmidOnline(const PmidName& /*online_pmid_name*/) {}
void DataManagerValue::SetPmidOffline(const PmidName& /*offline_pmid_name*/) {}

DataManagerValue::serialised_type DataManagerValue::Serialise() const {
  if ((subscribers == 0) || online_pmid_name.empty())
    ThrowError(CommonErrors::uninitialised);  // Cannot serialise if not a complete db value

  protobuf::DataManagerValue metadata_value_proto;
  metadata_value_proto.set_size(data_size);
  metadata_value_proto.set_subscribers(subscribers);
  for (const auto& i: online_pmid_name)
    metadata_value_proto.add_online_pmid_name(i->string());
  for (const auto& i: offline_pmid_name)
    metadata_value_proto.add_offline_pmid_name(i->string());
  assert(metadata_value_proto.IsInitialized());
  return serialised_type(NonEmptyString(metadata_value_proto.SerializeAsString()));
}

bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs) {
  return lhs.data_size == rhs.data_size &&
         lhs.subscribers == rhs.subscribers &&
         lhs.online_pmid_name == rhs.online_pmid_name &&
         lhs.offline_pmid_name == rhs.offline_pmid_name;
}

} // delete_me

}  // namespace vault

}  // namespace maidsafe
