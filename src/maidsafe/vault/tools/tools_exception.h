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

#ifndef MAIDSAFE_VAULT_TOOLS_TOOLS_EXCEPTION_H_
#define MAIDSAFE_VAULT_TOOLS_TOOLS_EXCEPTION_H_

#include <exception>
#include <string>

namespace maidsafe {

namespace vault {

namespace tools {

class ToolsException: public std::exception {
 public:
  explicit ToolsException(const std::string& message) : message_(message) {}
  virtual const char* what() const throw() { return message_.c_str(); }
  virtual ~ToolsException() throw() {}

 private:
  std::string message_;
};

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TOOLS_TOOLS_EXCEPTION_H_
