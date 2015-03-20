#==================================================================================================#
#                                                                                                  #
#  Copyright 2012 MaidSafe.net limited                                                             #
#                                                                                                  #
#  This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,        #
#  version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which    #
#  licence you accepted on initial access to the Software (the "Licences").                        #
#                                                                                                  #
#  By contributing code to the MaidSafe Software, or to this project generally, you agree to be    #
#  bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root        #
#  directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available   #
#  at: http://www.maidsafe.net/licenses                                                            #
#                                                                                                  #
#  Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed    #
#  under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF   #
#  ANY KIND, either express or implied.                                                            #
#                                                                                                  #
#  See the Licences for the specific language governing permissions and limitations relating to    #
#  use of the MaidSafe Software.                                                                   #
#                                                                                                  #
#==================================================================================================#


set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/standard_setup.cmake")
  cmake_minimum_required(VERSION 2.8)  # To suppress warning cluttering error message
  set(Msg "\nThis project can currently only be build as part of the MaidSafe super-project.  For")
  set(Msg "${Msg} full details, see https://github.com/maidsafe/MaidSafe/wiki/Build-Instructions\n")
  message(FATAL_ERROR "${Msg}")
endif()

project(vault)

include(../../cmake_modules/standard_setup.cmake)


#==================================================================================================#
# Set up all files as GLOBs                                                                        #
#==================================================================================================#
set(VaultSourcesDir ${PROJECT_SOURCE_DIR}/src/maidsafe/vault)

ms_glob_dir(DataManager ${VaultSourcesDir}/data_manager "Data Manager")
ms_glob_dir(MaidManager ${VaultSourcesDir}/maid_manager "Maid Manager")
ms_glob_dir(PmidManager ${VaultSourcesDir}/pmid_manager "Pmid Manager")
ms_glob_dir(PmidNode ${VaultSourcesDir}/pmid_node "Pmid Node")
ms_glob_dir(MpidManager ${VaultSourcesDir}/mpid_manager "Mpid Manager")

ms_glob_dir(Vault ${VaultSourcesDir} Vault)

list(REMOVE_ITEM VaultAllFiles ${VaultSourcesDir}/vault_main.cc)
ms_glob_dir(VaultTests ${VaultSourcesDir}/tests "Tests")


#==================================================================================================#
# Define MaidSafe libraries and executables                                                        #
#==================================================================================================#
ms_add_static_library(maidsafe_vault ${VaultAllFiles} ${DataManagerAllFiles}
                                     ${MaidManagerAllFiles} ${PmidManagerAllFiles}
                                     ${PmidNodeAllFiles} ${MpidManagerAllFiles})
target_include_directories(maidsafe_vault PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(maidsafe_vault maidsafe_routing)

ms_add_executable(vault Production ${VaultSourcesDir}/vault_main.cc)
target_include_directories(vault PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(vault maidsafe_vault)

if(INCLUDE_TESTS)
  ms_add_executable(test_vault "Tests/Vault" ${VaultTestsAllFiles})
  target_include_directories(test_vault PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(test_vault maidsafe_vault maidsafe_test)
endif()

ms_rename_outdated_built_exes()


#==================================================================================================#
# Set compiler and linker flags                                                                    #
#==================================================================================================#
include(standard_flags)


#==================================================================================================#
# Tests                                                                                            #
#==================================================================================================#
if(INCLUDE_TESTS)
  ms_add_default_tests()
  ms_add_gtests(test_vault)
  ms_test_summary_output()
endif()
