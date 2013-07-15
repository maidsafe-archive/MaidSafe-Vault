/* Copyright 2013 MaidSafe.net limited

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

#include "maidsafe/vault/version_manager/key.h"

//#include <algorithm>
//#include <tuple>
//
//#include "maidsafe/vault/utils.h"
//#include "maidsafe/vault/version_manager/version_manager_key.pb.h"

namespace maidsafe {

namespace vault {


//VersionManagerKey::VersionManagerKey(const DataNameVariant& name,
//                                     const Identity& originator)
//    : data_name_(data_name),
//      originator_(originator) {}
//
//
//template <typename Persona>
//Key::Key(const std::string& serialised_key) : name_(), originator_() {
//  protobuf::VersionManagerKey key_proto;
//  key_proto.ParseFromString(serialised_key);
//  name = GetDataNameVariant(static_cast<DataTagValue>(key_proto.type()),
//                            Identity(key_proto.name()));
//  originator = Identity(key_proto.originator());
//}
//
//// TODO requires a fixedwidth string type
////template <>
////Key<VersionManager>::Key(const std::string& serialised_key)
////    : data_name_(),
////      originator_() {
////  std::string name(serialised_key.substr(0, NodeId::kSize));
////  std::string type_as_string(serialised_key.substr(NodeId::kSize, kPaddedWidth_));
////  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth_>(type_as_string)));
////  data_name_ = GetDataNameVariant(type, Identity(name));
////  originator_ = Identity(serialised_key.substr(NodeId::kSize + kPaddedWidth_));
////  if (routing_.IsNodeIdInGroupRange(NodeId(name)) != routing::GroupRangeStatus::kInRange)
////    ThrowError(RoutingErrors::not_in_range);
////}
//
//
//template <>
//std::string Key<VersionManagerKey>::Serialise() const {
//  protobuf::VersionManagerKey key_proto;
//  static GetTagValueAndIdentityVisitor visitor;
//  auto result(boost::apply_visitor(visitor, name));
//  key_proto.set_name(result.second.string());
//  key_proto.set_type(static_cast<int32_t>(result.first));
//  key_proto.set_originator(originator.string());
//}
//
//std::string Key<VersionManagerKey>::ToFixedWidthString() const {
//  static GetTagValueAndIdentityVisitor visitor;
//  auto result(boost::apply_visitor(visitor, data_name_));
//  return std::string(
//      result.second.string() +
//      detail::ToFixedWidthString<kPaddedWidth_>(static_cast<uint32_t>(result.first)) +
//      originator_.string());
//}


}  // namespace vault

}  // namespace maidsafe
