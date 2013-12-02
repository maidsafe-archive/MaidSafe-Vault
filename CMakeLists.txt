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
glob_dir(CacheHandlerTests ${VaultSourcesDir}/cache_handler/tests "Tests")
glob_dir(DataManagerTests ${VaultSourcesDir}/data_manager/tests "Tests")
glob_dir(MaidManagerTests ${VaultSourcesDir}/maid_manager/tests "Tests")
#glob_dir(MpidManagerTests ${VaultSourcesDir}/mpid_manager/tests "Tests")
glob_dir(PmidManagerTests ${VaultSourcesDir}/pmid_manager/tests "Tests")
glob_dir(PmidNodeTests ${VaultSourcesDir}/pmid_node/tests "Tests")
glob_dir(VersionHandlerTests ${VaultSourcesDir}/version_handler/tests "Tests")
glob_dir(VaultTools ${VaultSourcesDir}/tools "Tools")
set(TestsMain ${VaultSourcesDir}/tests/tests_main.cc)

set(VaultGTestsAllFiles ${VaultTestsAllFiles})
set(VaultTestsAllFiles ${PmidNodeTestsAllFiles}
                       ${PmidManagerTestsAllFiles}
                       ${DataManagerTestsAllFiles}
                       ${VersionHandlerTestsAllFiles}
                       ${CacheHandlerTestsAllFiles}
                       ${MaidManagerTestsAllFiles}
                       ${VaultSourcesDir}/tests/db_test.cc)

list(REMOVE_ITEM VaultGTestsAllFiles ${VaultTestsAllFiles})
list(APPEND VaultTestsAllFiles ${TestsMain}  ${VaultSourcesDir}/tests/tests_utils.h
                                             ${VaultSourcesDir}/tests/tests_utils.cc )
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
target_include_directories(maidsafe_vault
  PUBLIC ${PROJECT_SOURCE_DIR}/include
  PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)

ms_add_executable(lifestuff_vault Production ${VaultSourcesDir}/vault_main.cc)
target_include_directories(lifestuff_vault PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)

target_link_libraries(maidsafe_vault maidsafe_nfs_vault maidsafe_routing maidsafe_common maidsafe_private leveldb)
target_link_libraries(lifestuff_vault maidsafe_vault maidsafe_lifestuff_manager)
if(MaidsafeTesting)
  ms_add_static_library(vault_mal ${VaultAllFiles}
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
  target_link_libraries(maidsafe_vault_mal maidsafe_nfs_vault maidsafe_routing maidsafe_common maidsafe_private leveldb)
  target_compile_definitions(maidsafe_vault_mal PRIVATE USE_MAL_BEHAVIOUR)
  ms_add_executable(lifestuff_vault_mal Production ${VaultSourcesDir}/vault_main.cc)
  target_include_directories(lifestuff_vault_mal PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_link_libraries(lifestuff_vault_mal maidsafe_vault_mal maidsafe_lifestuff_manager)
  target_compile_definitions(lifestuff_vault_mal PRIVATE USE_MAL_BEHAVIOUR)

  ms_add_executable(test_vault "Tests/Vault" ${VaultTestsAllFiles})
  target_include_directories(test_vault PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_link_libraries(test_vault maidsafe_vault)
  ms_add_executable(TESTvault "Tests/Vault" ${VaultGTestsAllFiles})
  target_include_directories(TESTvault PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_link_libraries(TESTvault maidsafe_vault)
  ms_add_executable(vault_key_helper "Tools/Vault" ${VaultToolsAllFiles})
  target_include_directories(vault_key_helper PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(vault_key_helper maidsafe_routing maidsafe_nfs_vault maidsafe_private)
endif()

rename_outdated_built_exes()


#==================================================================================================#
# Set compiler and linker flags                                                                    #
#==================================================================================================#
include(standard_flags)

if(MSVC AND MaidsafeTesting)
  set_target_properties(maidsafe_vault TESTvault PROPERTIES COMPILE_FLAGS "/bigobj")
  set_target_properties(maidsafe_vault_mal TESTvault PROPERTIES COMPILE_FLAGS "/bigobj")
endif()


#==================================================================================================#
# Tests                                                                                            #
#==================================================================================================#
if(MaidsafeTesting)
  add_style_test()
  add_catch_tests(test_vault)
  add_gtests(TESTvault)
  add_project_experimental()
  test_summary_output()
endif()
