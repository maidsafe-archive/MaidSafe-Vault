/*  Copyright 2012 MaidSafe.net limited

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

#include "maidsafe/vault/key_utils.h"


namespace maidsafe {

namespace vault {

namespace detail {

const int PaddedWidth::value;

template<>
std::string ToFixedWidthString<1>(uint32_t number) {
  assert(number < 256);
  return std::string(1, static_cast<char>(number));
}

template<>
uint32_t FromFixedWidthString<1>(const std::string& number_as_string) {
  assert(number_as_string.size() == 1U);
  return static_cast<uint32_t>(static_cast<unsigned char>(number_as_string[0]));
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
