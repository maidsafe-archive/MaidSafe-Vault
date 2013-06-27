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

#include "maidsafe/common/error.h"

#include "maidsafe/vault/tools/commander.h"
#include "maidsafe/vault/tools/tools_exception.h"

int main(int argc, char* argv[]) {
  maidsafe::log::Logging::Instance().Initialise(argc, argv);

  std::cout << maidsafe::vault::tools::kHelperVersion << std::endl;

  try {
    maidsafe::vault::tools::Commander commander(12);
    commander.AnalyseCommandLineOptions(argc, argv);
  }
  catch(const maidsafe::vault::tools::ToolsException& exception) {
    std::cout << "Tools Exception: " << exception.what() << std::endl;
    return -1;
  }
  catch(const maidsafe::maidsafe_error& exception) {
    std::cout << "Error: " << exception.what() << std::endl;
    return -2;
  }
  catch(...) {
    std::cout << "Unexpected exception." << std::endl;
    return -3;
  }

  return 0;
}
