/*  Copyright 2014 MaidSafe.net limited

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

#include "maidsafe/vault/tests/chunk_store_test_utils.h"

#include "maidsafe/common/utils.h"
#include "maidsafe/common/data_types/data_type_values.h"
#include "maidsafe/common/data_types/immutable_data.h"
#include "maidsafe/common/data_types/mutable_data.h"

#include "maidsafe/passport/types.h"

namespace maidsafe {

namespace vault {

namespace test {

void AddRandomKeyValuePairs(std::vector<std::pair<DataNameVariant, NonEmptyString>>& container,
                            uint32_t number, uint32_t size) {
  NonEmptyString value;
  for (uint32_t i = 0; i != number; ++i) {
    auto type_number = static_cast<DataTagValue>(RandomUint32() % MAIDSAFE_DATA_TYPES_SIZE);
    value = NonEmptyString(RandomAlphaNumericString(size));
    switch (type_number) {
      case DataTagValue::kAnmaidValue: {
        passport::PublicAnmaid::Name key(Identity(crypto::Hash<crypto::SHA512>(value)));
        container.push_back(std::make_pair(key, value));
        break;
      }
      case DataTagValue::kMaidValue: {
        passport::PublicMaid::Name key(Identity(crypto::Hash<crypto::SHA512>(value)));
        container.push_back(std::make_pair(key, value));
        break;
      }
      case DataTagValue::kAnpmidValue: {
        passport::PublicAnpmid::Name key(Identity(crypto::Hash<crypto::SHA512>(value)));
        container.push_back(std::make_pair(key, value));
        break;
      }
      case DataTagValue::kPmidValue: {
        passport::PublicPmid::Name key(Identity(crypto::Hash<crypto::SHA512>(value)));
        container.push_back(std::make_pair(key, value));
        break;
      }
      case DataTagValue::kAnmpidValue: {
        passport::PublicAnmpid::Name key(Identity(crypto::Hash<crypto::SHA512>(value)));
        container.push_back(std::make_pair(key, value));
        break;
      }
      case DataTagValue::kMpidValue: {
        passport::PublicMpid::Name key(Identity(crypto::Hash<crypto::SHA512>(value)));
        container.push_back(std::make_pair(key, value));
        break;
      }
      case DataTagValue::kImmutableDataValue: {
        ImmutableData::Name key(Identity(crypto::Hash<crypto::SHA512>(value)));
        container.push_back(std::make_pair(key, value));
        break;
      }
      case DataTagValue::kMutableDataValue: {
        MutableData::Name key(Identity(crypto::Hash<crypto::SHA512>(value)));
        container.push_back(std::make_pair(key, value));
        break;
      }
    }
  }
}

DataNameVariant GetRandomDataNameType() {
  auto type_number = static_cast<DataTagValue>(RandomUint32() % MAIDSAFE_DATA_TYPES_SIZE);
  switch (type_number) {
    case DataTagValue::kAnmaidValue:
      return passport::PublicAnmaid::Name();
    case DataTagValue::kMaidValue:
      return passport::PublicMaid::Name();
    case DataTagValue::kAnpmidValue:
      return passport::PublicAnpmid::Name();
    case DataTagValue::kPmidValue:
      return passport::PublicPmid::Name();
    case DataTagValue::kAnmpidValue:
      return passport::PublicAnmpid::Name();
    case DataTagValue::kMpidValue:
      return passport::PublicMpid::Name();
    case DataTagValue::kImmutableDataValue:
      return ImmutableData::Name();
    case DataTagValue::kMutableDataValue:
      return MutableData::Name();
    default:
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::unknown));
  }
}


}  // namespace test

}  // namespace vault

}  // namespace maidsafe

