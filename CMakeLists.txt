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


#==================================================================================================#
# Set up all files as GLOBs                                                                        #
#==================================================================================================#
set(VaultSourcesDir ${PROJECT_SOURCE_DIR}/src/maidsafe/vault)

set(OutputFile ${PROJECT_BINARY_DIR}/generated_message_types/maidsafe/vault/message_types.h)
set(InputFile ${PROJECT_SOURCE_DIR}/cmake/message_types.h.in)
file(GLOB_RECURSE MetaFiles "${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.message_types.meta")
ms_set_meta_files_custom_commands("${OutputFile}" "${InputFile}" "${MetaFiles}" "Vault Header Files" "Vault CMake Files")

ms_glob_dir(Vault ${VaultSourcesDir} Vault)
list(REMOVE_ITEM VaultAllFiles ${VaultSourcesDir}/vault_main.cc)
ms_glob_dir(CacheHandler ${VaultSourcesDir}/cache_handler "Cache Handler\\\\")
ms_glob_dir(DataManager ${VaultSourcesDir}/data_manager "Data Manager\\\\")
ms_glob_dir(MaidManager ${VaultSourcesDir}/maid_manager "Maid Manager\\\\")
ms_glob_dir(MpidManager ${VaultSourcesDir}/mpid_manager "Mpid Manager\\\\")
ms_glob_dir(PmidManager ${VaultSourcesDir}/pmid_manager "Pmid Manager\\\\")
ms_glob_dir(PmidNode ${VaultSourcesDir}/pmid_node "Pmid Node\\\\")
ms_glob_dir(VersionHandler ${VaultSourcesDir}/version_handler "Version Handler\\\\")
ms_glob_dir(VaultTests ${VaultSourcesDir}/tests "Tests")
ms_glob_dir(CacheHandlerTests ${VaultSourcesDir}/cache_handler/tests "Tests")
ms_glob_dir(DataManagerTests ${VaultSourcesDir}/data_manager/tests "Tests")
ms_glob_dir(MaidManagerTests ${VaultSourcesDir}/maid_manager/tests "Tests")
#ms_glob_dir(MpidManagerTests ${VaultSourcesDir}/mpid_manager/tests "Tests")
ms_glob_dir(PmidManagerTests ${VaultSourcesDir}/pmid_manager/tests "Tests")
ms_glob_dir(PmidNodeTests ${VaultSourcesDir}/pmid_node/tests "Tests")
ms_glob_dir(VersionHandlerTests ${VaultSourcesDir}/version_handler/tests "Tests")
ms_glob_dir(VaultTools ${VaultSourcesDir}/tools "Tools")
set(TestsMain ${VaultSourcesDir}/tests/tests_main.cc)

set(VaultGTestsAllFiles ${VaultTestsAllFiles})
set(VaultTestsAllFiles ${PmidNodeTestsAllFiles}
                       ${PmidManagerTestsAllFiles}
                       ${DataManagerTestsAllFiles}
                       ${VersionHandlerTestsAllFiles}
                       ${CacheHandlerTestsAllFiles}
                       ${MaidManagerTestsAllFiles}
                       ${VaultSourcesDir}/tests/db_test.cc)

set(VaultNetworkTestFiles ${VaultSourcesDir}/tests/vault_network.h
                          ${VaultSourcesDir}/tests/vault_network.cc
                          ${VaultSourcesDir}/tests/vault_network_test.cc
                          ${VaultSourcesDir}/version_handler/tests/version_handler_test.cc)

list(REMOVE_ITEM VaultTestsAllFiles ${VaultNetworkTestFiles})
list(REMOVE_ITEM VaultGTestsAllFiles ${VaultTestsAllFiles} ${VaultNetworkTestFiles})
list(APPEND VaultNetworkTestFiles ${TestsMain} ${VaultSourcesDir}/tests/tests_utils.h
                                               ${VaultSourcesDir}/tests/tests_utils.cc )
list(APPEND VaultTestsAllFiles ${TestsMain} ${VaultSourcesDir}/tests/tests_utils.h
                                            ${VaultSourcesDir}/tests/tests_utils.cc )


#==================================================================================================#
# Define MaidSafe libraries and executables                                                        #
#==================================================================================================#
ms_add_static_library(maidsafe_vault ${VaultAllFiles}
                                     ${CacheHandlerAllFiles}
                                     ${DataManagerAllFiles}
                                     ${MaidManagerAllFiles}
                                     ${MpidManagerAllFiles}
                                     ${PmidManagerAllFiles}
                                     ${PmidNodeAllFiles}
                                     ${VersionHandlerAllFiles}
                                     ${OutputFile} ${InputFile} ${MetaFiles})
target_include_directories(maidsafe_vault
  PUBLIC ${PROJECT_SOURCE_DIR}/include
  PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
# TODO - Investigate why boost variant requires this warning to be disabled.
target_compile_options(maidsafe_vault PUBLIC $<$<AND:$<BOOL:${MSVC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:/wd4702>)

ms_add_executable(vault Production ${VaultSourcesDir}/vault_main.cc)
target_include_directories(vault PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)

target_link_libraries(maidsafe_vault maidsafe_nfs_vault maidsafe_routing maidsafe_common maidsafe_private leveldb)
target_link_libraries(vault maidsafe_vault maidsafe_client_manager)
if(MaidsafeTesting)
  ms_add_static_library(maidsafe_vault_mal ${VaultAllFiles}
                                           ${CacheHandlerAllFiles}
                                           ${DataManagerAllFiles}
                                           ${MaidManagerAllFiles}
                                           ${MpidManagerAllFiles}
                                           ${PmidManagerAllFiles}
                                           ${PmidNodeAllFiles}
                                           ${VersionHandlerAllFiles}
                                           ${OutputFile} ${InputFile} ${MetaFiles})
  target_include_directories(maidsafe_vault_mal
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  # TODO - Investigate why boost variant requires this warning to be disabled.
  target_compile_options(maidsafe_vault_mal PUBLIC $<$<AND:$<BOOL:${MSVC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:/wd4702>)
  target_link_libraries(maidsafe_vault_mal maidsafe_nfs_vault maidsafe_routing maidsafe_common maidsafe_private leveldb)
  target_compile_definitions(maidsafe_vault_mal PRIVATE USE_MAL_BEHAVIOUR)
  ms_add_executable(vault_mal Production ${VaultSourcesDir}/vault_main.cc)
  target_include_directories(vault_mal PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_link_libraries(vault_mal maidsafe_vault_mal maidsafe_client_manager)
  target_compile_definitions(vault_mal PRIVATE USE_MAL_BEHAVIOUR)

  ms_add_executable(test_vault "Tests/Vault" ${VaultTestsAllFiles})
  target_include_directories(test_vault PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_link_libraries(test_vault maidsafe_vault)
  ms_add_executable(TESTvault "Tests/Vault" ${VaultGTestsAllFiles})
  target_include_directories(TESTvault PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_link_libraries(TESTvault maidsafe_vault)
  ms_add_executable(TESTvault_network "Tests/Vault" ${VaultNetworkTestFiles})
  target_include_directories(TESTvault_network PRIVATE ${PROJECT_SOURCE_DIR}/src
                                                       ${PROJECT_BINARY_DIR}/generated_message_types
                                                       ${VaultSourcesDir}/tests)
  target_link_libraries(TESTvault_network maidsafe_vault)
  ms_add_executable(vault_key_helper "Tools/Vault" ${VaultToolsAllFiles})
  target_include_directories(vault_key_helper PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(vault_key_helper maidsafe_routing maidsafe_nfs_vault maidsafe_private)
  set_target_properties(maidsafe_vault_mal vault_mal PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
endif()

ms_rename_outdated_built_exes()


#==================================================================================================#
# Set compiler and linker flags                                                                    #
#==================================================================================================#
include(standard_flags)

if(MSVC AND MaidsafeTesting)
  set_property(TARGET maidsafe_vault TESTvault TESTvault_network maidsafe_vault_mal APPEND PROPERTY COMPILE_FLAGS "/bigobj")
endif()


#==================================================================================================#
# Tests                                                                                            #
#==================================================================================================#
if(MaidsafeTesting)
  ms_add_style_test()
  ms_add_catch_tests(test_vault)
  ms_add_gtests(TESTvault)
  ms_add_gtests(TESTvault_network)
  ms_add_project_experimental()
  ms_test_summary_output()
endif()
