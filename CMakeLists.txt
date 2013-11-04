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

project(vault)

include(../../cmake_modules/standard_setup.cmake)

include_directories(${common_SOURCE_DIR}/include)
include_directories(${passport_SOURCE_DIR}/include)
include_directories(${private_SOURCE_DIR}/include)
include_directories(${rudp_SOURCE_DIR}/include)
include_directories(${routing_SOURCE_DIR}/include)
include_directories(${nfs_SOURCE_DIR}/include)


#==================================================================================================#
# Set up all files as GLOBs                                                                        #
#==================================================================================================#
set(VaultSourcesDir ${PROJECT_SOURCE_DIR}/src/maidsafe/vault)

set(OutputFile ${PROJECT_BINARY_DIR}/generated_message_types/maidsafe/vault/message_types.h)
include_directories(${PROJECT_BINARY_DIR}/generated_message_types)
set(InputFile ${PROJECT_SOURCE_DIR}/cmake/message_types.h.in)
file(GLOB_RECURSE MetaFiles "${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.message_types.meta")
set_meta_files_custom_commands("${OutputFile}" "${InputFile}" "${MetaFiles}" "Vault Header Files" "Vault CMake Files")

glob_dir(Vault ${VaultSourcesDir} Vault)
list(REMOVE_ITEM VaultAllFiles ${VaultSourcesDir}/vault_main.cc)
glob_dir(CacheHandler ${VaultSourcesDir}/cache_handler "Cache Handler\\\\")
glob_dir(DataManager ${VaultSourcesDir}/data_manager "Data Manager\\\\")
glob_dir(MaidManager ${VaultSourcesDir}/maid_manager "Maid Manager\\\\")
glob_dir(MpidManager ${VaultSourcesDir}/mpid_manager "Mpid Manager\\\\")
glob_dir(PmidManager ${VaultSourcesDir}/pmid_manager "Pmid Manager\\\\")
glob_dir(PmidNode ${VaultSourcesDir}/pmid_node "Pmid Node\\\\")
glob_dir(VersionHandler ${VaultSourcesDir}/version_handler "Version Handler\\\\")
glob_dir(VaultTests ${VaultSourcesDir}/tests "Tests")
glob_dir(VaultTools ${VaultSourcesDir}/tools "Tools")
set(TestsMain ${VaultSourcesDir}/tests/tests_main.cc)

set(VaultGTestsAllFiles ${VaultTestsAllFiles})
set(VaultTestsAllFiles ${VaultSourcesDir}/tests/db_test.cc
                       ${VaultSourcesDir}/tests/group_db_test.cc)
list(REMOVE_ITEM VaultGTestsAllFiles ${VaultTestsAllFiles})
list(APPEND VaultTestsAllFiles ${TestsMain})

#==================================================================================================#
# Define MaidSafe libraries and executables                                                        #
#==================================================================================================#
ms_add_static_library(vault ${VaultAllFiles}
                            ${CacheHandlerAllFiles}
                            ${DataManagerAllFiles}
                            ${MaidManagerAllFiles}
                            ${MpidManagerAllFiles}
                            ${PmidManagerAllFiles}
                            ${PmidNodeAllFiles}
                            ${VersionHandlerAllFiles}
                            ${OutputFile} ${InputFile} ${MetaFiles})
ms_add_executable(lifestuff_vault Production ${VaultSourcesDir}/vault_main.cc)

target_link_libraries(maidsafe_vault maidsafe_nfs_vault maidsafe_routing maidsafe_common maidsafe_private leveldb)
target_link_libraries(lifestuff_vault maidsafe_vault maidsafe_lifestuff_manager)
if(MaidsafeTesting)
  ms_add_executable(TESTvault "Tests/Vault" ${VaultGTestsAllFiles})
  target_link_libraries(TESTvault maidsafe_vault)
  ms_add_executable(test_vault "Tests/Vault" ${VaultTestsAllFiles})
  target_link_libraries(test_vault maidsafe_vault)
  ms_add_executable(vault_key_helper Tools ${VaultToolsAllFiles})
  target_link_libraries(vault_key_helper maidsafe_routing maidsafe_nfs_vault maidsafe_private)
endif()

rename_outdated_built_exes()


#==================================================================================================#
# Set compiler and linker flags                                                                    #
#==================================================================================================#
include(standard_flags)

if(MSVC AND MaidsafeTesting)
  set_target_properties(maidsafe_vault TESTvault PROPERTIES COMPILE_FLAGS "/bigobj")
endif()


#==================================================================================================#
# Tests                                                                                            #
#==================================================================================================#
if(MaidsafeTesting)
  add_style_test()
  add_gtests(TESTvault)
  add_catch_tests(test_vault)
  add_project_experimental()
  test_summary_output()
endif()
