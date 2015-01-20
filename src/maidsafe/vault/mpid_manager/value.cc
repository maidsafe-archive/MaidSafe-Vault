/*  Copyright 2015 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/mpid_manager/value.h"

#include <utility>
#include <limits>

#include "maidsafe/vault/utils.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/vault/mpid_manager/mpid_manager.pb.h"

namespace maidsafe {

namespace vault {

//MpidManagerValue::MpidManagerValue() : data() {}

MpidManagerValue::MpidManagerValue(const ImmutableData& data_in) : data(data_in) {}

MpidManagerValue::MpidManagerValue(const MpidManagerValue& other) : data(other.data) {}

MpidManagerValue::MpidManagerValue(MpidManagerValue&& other) : data(std::move(other.data)) {}

MpidManagerValue& MpidManagerValue::operator=(MpidManagerValue other) {
  swap(*this, other);
  return *this;
}

MpidManagerValue::MpidManagerValue(const std::string& serialised_value)
  : data([&serialised_value]()->NonEmptyString {
             protobuf::MpidManagerValue mpid_manager_value_proto;
             if (!mpid_manager_value_proto.ParseFromString(serialised_value)) {
               LOG(kError) << "Failed to read or parse serialised mpid manager value";
               BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
             }
             return NonEmptyString(mpid_manager_value_proto.data());
         }()) {}

std::string MpidManagerValue::Serialise() const {
  protobuf::MpidManagerValue mpid_manager_value_proto;
  mpid_manager_value_proto.set_data(data.data().string());
  return mpid_manager_value_proto.SerializeAsString();
}

MpidManagerValue MpidManagerValue::Resolve(const std::vector<MpidManagerValue>& values) {
  size_t size(values.size());
  if (size < (routing::Parameters::group_size + 1) / 2)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::too_few_entries_to_resolve));

  std::vector<std::pair<MpidManagerValue, unsigned int>> stats;
  for (const auto& value : values) {
    auto iter(std::find_if(std::begin(stats), std::end(stats),
                           [&](const std::pair<MpidManagerValue, unsigned int>& pair) {
                             return value == pair.first;
                           }));
    if (iter == std::end(stats))
      stats.push_back(std::make_pair(value, 1));
    else
      iter->second++;
  }

  auto max_iter(std::begin(stats));
  for (auto iter(std::begin(stats)); iter != std::end(stats); ++iter)
    max_iter = (iter->second > max_iter->second) ? iter : max_iter;

  if (max_iter->second == (routing::Parameters::group_size + 1) / 2)
    return max_iter->first;

  if (values.size() == routing::Parameters::group_size - 1)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));

  BOOST_THROW_EXCEPTION(MakeError(VaultErrors::too_few_entries_to_resolve));
}

void swap(MpidManagerValue& lhs, MpidManagerValue& rhs) {
  using std::swap;
  swap(lhs.data, rhs.data);
}

bool operator==(const MpidManagerValue& lhs, const MpidManagerValue& rhs) {
  return lhs.data.data() == rhs.data.data();
}

}  // namespace vault

}  // namespace maidsafe
