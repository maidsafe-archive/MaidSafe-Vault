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

#ifndef MAIDSAFE_VAULT_KEY_UTILS_H_
#define MAIDSAFE_VAULT_KEY_UTILS_H_

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>


namespace maidsafe {

namespace vault {

namespace detail {

// This allows for 2 ^ (8 * value) different data types
struct PaddedWidth {
  static const int value = 1;
};

template<int width>
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

template<int width>
uint32_t FromFixedWidthString(const std::string& number_as_string) {
  static_assert(width > 0 && width < 5, "width must be 1, 2, 3, or 4.");
  assert(static_cast<int>(number_as_string.size()) == width);
  uint32_t result(0), factor(1);
  for (int i(0); i != width; ++i) {
    result += (static_cast<unsigned char>(number_as_string[width - i - 1]) * factor);
    factor *= 256;
  }
  assert(result < std::pow(256, width));
  return result;
}

template<>
std::string ToFixedWidthString<1>(uint32_t number);

template<>
uint32_t FromFixedWidthString<1>(const std::string& number_as_string);

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_KEY_UTILS_H_
