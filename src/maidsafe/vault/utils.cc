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

#include "maidsafe/vault/utils.h"

#include <string>

#include "boost/filesystem/operations.hpp"

namespace maidsafe {

namespace vault {

namespace detail {

const DataTypeId TypeId<ImmutableData>::value = DataTypeId(0);
const DataTypeId TypeId<MutableData>::value = DataTypeId(1);
const DataTypeId TypeId<passport::PublicAnmaid>::value =
    DataTypeId(passport::PublicAnmaid::Tag::type_id);
const DataTypeId TypeId<passport::PublicMaid>::value =
    DataTypeId(passport::PublicMaid::Tag::type_id);
const DataTypeId TypeId<passport::PublicAnpmid>::value =
    DataTypeId(passport::PublicAnpmid::Tag::type_id);
const DataTypeId TypeId<passport::PublicPmid>::value =
    DataTypeId(passport::PublicPmid::Tag::type_id);
const DataTypeId TypeId<passport::PublicAnmpid>::value =
    DataTypeId(passport::PublicAnmpid::Tag::type_id);
const DataTypeId TypeId<passport::PublicMpid>::value =
    DataTypeId(passport::PublicMpid::Tag::type_id);

}  // namespace detail

template <>
std::string ToFixedWidthString<1>(uint32_t number) {
  assert(number < 256);
  return std::string(1, static_cast<char>(number));
}

void InitialiseDirectory(const boost::filesystem::path& directory) {
  if (boost::filesystem::exists(directory)) {
    if (!boost::filesystem::is_directory(directory))
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::not_a_directory));
  } else {
    boost::filesystem::create_directory(directory);
  }
}

boost::filesystem::path UniqueDbPath(const boost::filesystem::path& vault_root_dir) {
  boost::filesystem::path db_root_path(vault_root_dir / "db");
  InitialiseDirectory(db_root_path);
  return (db_root_path / boost::filesystem::unique_path());
}

size_t Parameters::min_pmid_holders = 4;

}  // namespace vault

}  // namespace maidsafe
