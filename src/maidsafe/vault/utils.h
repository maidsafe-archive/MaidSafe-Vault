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

#ifndef MAIDSAFE_VAULT_UTILS_H_
#define MAIDSAFE_VAULT_UTILS_H_

#include <string>
#include <vector>

#include "boost/filesystem.hpp"

#include "maidsafe/common/identity.h"
#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/immutable_data.h"
#include "maidsafe/common/data_types/mutable_data.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/routing/types.h"
#include "maidsafe/routing/source_address.h"

namespace maidsafe {

namespace vault {

namespace detail {

// Workaround until new lib with all data types is available
template <typename DataType>
struct TypeId;

template <>
struct TypeId<ImmutableData> {
  static const DataTypeId value;
};

template <>
struct TypeId<MutableData> {
  static const DataTypeId value;
};

template <>
struct TypeId<passport::PublicAnmaid> {
  static const DataTypeId value;
};

template <>
struct TypeId<passport::PublicMaid> {
  static const DataTypeId value;
};

template <>
struct TypeId<passport::PublicAnpmid> {
  static const DataTypeId value;
};

template <>
struct TypeId<passport::PublicPmid> {
  static const DataTypeId value;
};

template <>
struct TypeId<passport::PublicAnmpid> {
  static const DataTypeId value;
};

template <>
struct TypeId<passport::PublicMpid> {
  static const DataTypeId value;
};

}  // namespace detail

void InitialiseDirectory(const boost::filesystem::path& directory);
boost::filesystem::path UniqueDbPath(const boost::filesystem::path& vault_root_dir);

struct PaddedWidth {
  static const int value = 1;
};

using FixedWidthString =
    maidsafe::detail::BoundedString<identity_size + PaddedWidth::value,
                                    identity_size + PaddedWidth::value, std::string>;
template <int width>
std::string ToFixedWidthString(uint32_t number) {
  static_assert(width > 0 && width < 5, "width must be 1, 2, 3, or 4.");
  assert(number < std::pow(256, width));
  std::string result(width, 0);
  for (int i(0); i != width; ++i) {
    result[width - i - 1] = static_cast<char>(number);
    number /= 256;
  }
  return result;
}

template <>
std::string ToFixedWidthString<1>(uint32_t number);

template <typename DataType>
std::string EncodeToString(const Identity& name) {
  const std::vector<byte>& raw_name(name.string());
  return FixedWidthString(std::string(raw_name.begin(), raw_name.end()) +
                          ToFixedWidthString<PaddedWidth::value>(
                              detail::TypeId<DataType>::value.data)).string();
}

struct Parameters {
  static size_t min_pmid_holders;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_H_

