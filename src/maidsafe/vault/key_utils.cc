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
