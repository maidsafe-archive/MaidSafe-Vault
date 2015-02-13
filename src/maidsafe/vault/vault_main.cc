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

#include <chrono>
#include <cstdint>
#include <future>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/vault.h"

int main(int argc, char* argv[]) {
  using maidsafe::vault::VaultFacade;
  int exit_code(0);
  try {
    auto unuseds(maidsafe::log::Logging::Instance().Initialise(argc, argv));
    if (unuseds.size() != 2U)
      BOOST_THROW_EXCEPTION(maidsafe::MakeError(maidsafe::CommonErrors::invalid_parameter));
//    uint16_t port{static_cast<uint16_t>(std::stoi(std::string{&unuseds[1][0]}))};
//    maidsafe::vault_manager::VaultInterface vault_interface{port};
//    VaultConfig vault_config{vault_interface.GetConfiguration()};

    LOG(kVerbose) << "Starting vault...";
    VaultFacade vault;
    LOG(kInfo) << "Vault running as " /*<< maidsafe::HexSubstr(vault_config.pmid.name().value)*/;
//    exit_code = vault_interface.WaitForExit(); // FIXME(Prakash)
  }
  catch (const maidsafe::maidsafe_error& error) {
//    LOG(kError) << "This is only designed to be invoked by VaultManager.";
    exit_code = maidsafe::ErrorToInt(error);
  }
  catch (const std::exception& /*e*/) {
//    LOG(kError) << "This is only designed to be invoked by VaultManager: " << e.what();
    exit_code =
        maidsafe::ErrorToInt(maidsafe::MakeError(maidsafe::CommonErrors::invalid_parameter));
  }
//  try {
//    VLOG(maidsafe::vault::VisualiserAction::kVaultStopped, exit_code);
//  }
//  catch (const maidsafe::maidsafe_error& err) {
//    if (err.code() == maidsafe::make_error_code(maidsafe::CommonErrors::unable_to_handle_request)) {
//      LOG(kWarning) << "Visualiser logging has not been initialised.";
//    } else {
//      LOG(kError) << boost::diagnostic_information(err);
//      throw;
//    }
//  }
  return exit_code;
}
