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

set(TestsMain ${VaultSourcesDir}/tests/tests_main.cc)
set(TestsFunctionalMain ${VaultSourcesDir}/tests/tests_func_main.cc)

set(VaultTests         ${VaultTestsAllFiles} 
                       ${PmidNodeTestsAllFiles}
                       ${PmidManagerTestsAllFiles}
                       ${DataManagerTestsAllFiles}
                       ${VersionHandlerTestsAllFiles}
                       ${CacheHandlerTestsAllFiles}
                       ${MaidManagerTestsAllFiles}
                       ${VaultSourcesDir}/tests/db_test.cc)

set(VaultFunctionalTestFiles ${VaultSourcesDir}/tests/vault_network.h
                             ${VaultSourcesDir}/tests/vault_network.cc
                             ${VaultSourcesDir}/tests/vault_func_test.cc
                             ${VaultSourcesDir}/cache_handler/tests/cache_handler_test.cc
                             ${VaultSourcesDir}/version_handler/tests/version_handler_test.cc
                             ${VaultSourcesDir}/pmid_manager/tests/pmid_manager_test.cc)

list(REMOVE_ITEM VaultTests ${VaultFunctionalTestFiles} ${TestsFunctionalMain})
list(APPEND VaultFunctionalTestFiles ${TestsFunctionalMain} ${VaultSourcesDir}/tests/tests_utils.h
                                     ${VaultSourcesDir}/tests/tests_utils.cc)
list(APPEND VaultTests ${TestsMain} ${VaultSourcesDir}/tests/tests_utils.h
                                    ${VaultSourcesDir}/tests/tests_utils.cc)


#==================================================================================================#
# Define MaidSafe libraries and executables                                                        #
#==================================================================================================#
ms_add_static_library(maidsafe_vault ${VaultAllFiles} ${OutputFile} ${InputFile} ${MetaFiles})
ms_add_static_library(maidsafe_vault_cache_handler ${CacheHandlerAllFiles} ${OutputFile} ${InputFile})
ms_add_static_library(maidsafe_vault_data_manager ${DataManagerAllFiles} ${OutputFile} ${InputFile})
ms_add_static_library(maidsafe_vault_maid_manager ${MaidManagerAllFiles} ${OutputFile} ${InputFile})
ms_add_static_library(maidsafe_vault_mpid_manager ${MpidManagerAllFiles} ${OutputFile} ${InputFile})
ms_add_static_library(maidsafe_vault_pmid_manager ${PmidManagerAllFiles} ${OutputFile} ${InputFile})
ms_add_static_library(maidsafe_vault_pmid_node ${PmidNodeAllFiles} ${OutputFile} ${InputFile})
ms_add_static_library(maidsafe_vault_version_handler ${VersionHandlerAllFiles} ${OutputFile} ${InputFile})

# Add custom target to trigger atomic generation of protobuf files
set(AllProtos
    ${VaultProtos}
    ${CacheHandlerProtos}
    ${DataManagerProtos}
    ${MaidManagerProtos}
    ${MpidManagerProtos}
    ${PmidManagerProtos}
    ${PmidNodeProtos}
    ${VersionHandlerProtos})
if(MSVC OR CMAKE_GENERATOR STREQUAL Xcode)
  add_custom_target(vault_protobuf_files_generator DEPENDS ${AllProtos} SOURCES ${AllProtos}
                    COMMENT "Generated all vault protocol buffer files.")
else()
  # Need to avoid use of SOURCES arg here - CMake bug.
  add_custom_target(vault_protobuf_files_generator DEPENDS ${AllProtos}
                    COMMENT "Generated all vault protocol buffer files.")
endif()
set_target_properties(vault_protobuf_files_generator PROPERTIES LABELS Vault FOLDER "MaidSafe/Libraries/Vault")

set(VaultPersonaLibs
    maidsafe_vault_cache_handler
    maidsafe_vault_data_manager
    maidsafe_vault_maid_manager
    maidsafe_vault_mpid_manager
    maidsafe_vault_pmid_manager
    maidsafe_vault_pmid_node
    maidsafe_vault_version_handler)
foreach(VaultLib maidsafe_vault ${VaultPersonaLibs})
  target_include_directories(${VaultLib}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_compile_options(${VaultLib} PRIVATE $<$<BOOL:${MSVC}>:/bigobj>)
  if(VaultLib STREQUAL "maidsafe_vault")
    target_link_libraries(${VaultLib} maidsafe_vault_manager maidsafe_nfs_vault leveldb)
  else()
    target_link_libraries(${VaultLib} maidsafe_vault maidsafe_nfs_vault leveldb)
  endif()
  add_dependencies(${VaultLib} vault_protobuf_files_generator)
endforeach()

# TODO - Investigate why boost variant requires this warning to be disabled.
target_compile_options(maidsafe_vault PUBLIC $<$<AND:$<BOOL:${MSVC}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:/wd4702>)

ms_add_executable(vault Production ${VaultSourcesDir}/vault_main.cc)
target_include_directories(vault PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)

target_link_libraries(maidsafe_vault ${VaultPersonaLibs})
target_link_libraries(vault maidsafe_vault)

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
  target_link_libraries(maidsafe_vault_mal maidsafe_vault_manager maidsafe_nfs_vault leveldb)
  target_compile_definitions(maidsafe_vault_mal PRIVATE USE_MAL_BEHAVIOUR)

  ms_add_executable(vault_mal "Tools/Vault" ${VaultSourcesDir}/vault_main.cc)
  target_include_directories(vault_mal PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_link_libraries(vault_mal maidsafe_vault_mal)
  target_compile_definitions(vault_mal PRIVATE USE_MAL_BEHAVIOUR)
  set_target_properties(maidsafe_vault_mal vault_mal PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)

  ms_add_executable(test_vault "Tests/Vault" ${VaultTests})
  target_include_directories(test_vault PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/generated_message_types)
  target_link_libraries(test_vault maidsafe_vault maidsafe_test)

  ms_add_executable(network_test_vault "Tests/Vault" ${VaultFunctionalTestFiles})
  target_include_directories(network_test_vault PRIVATE ${PROJECT_SOURCE_DIR}/src
                                                        ${PROJECT_BINARY_DIR}/generated_message_types
                                                        ${VaultSourcesDir}/tests)
  target_link_libraries(network_test_vault maidsafe_vault maidsafe_test)
endif()

ms_rename_outdated_built_exes()


#==================================================================================================#
# Set compiler and linker flags                                                                    #
#==================================================================================================#
include(standard_flags)

if(MSVC AND MaidsafeTesting)
  set_property(TARGET maidsafe_vault test_vault network_test_vault maidsafe_vault_mal APPEND PROPERTY COMPILE_FLAGS "/bigobj")
endif()
if(MSVC)
  target_compile_options(vault PUBLIC $<$<CONFIG:Release>: /bigobj>)
endif()


#==================================================================================================#
# Tests                                                                                            #
#==================================================================================================#
if(MaidsafeTesting)
  ms_add_default_tests()
  ms_add_gtests(test_vault)
  ms_add_network_gtests(network_test_vault)
  ms_test_summary_output()
endif()


#==================================================================================================#
# Package                                                                                          #
#==================================================================================================#
# install(TARGETS maidsafe_vault maidsafe_vault_cache_handler maidsafe_vault_data_manager maidsafe_vault_maid_manager
#                 maidsafe_vault_mpid_manager maidsafe_vault_pmid_manager maidsafe_vault_pmid_node maidsafe_vault_version_handler
#                 COMPONENT Development CONFIGURATIONS Debug Release ARCHIVE DESTINATION lib)
install(TARGETS vault COMPONENT Apps CONFIGURATIONS Debug RUNTIME DESTINATION bin/debug)
install(TARGETS vault COMPONENT Apps CONFIGURATIONS Release RUNTIME DESTINATION bin)

if(MaidsafeTesting)
  install(TARGETS test_vault network_test_vault COMPONENT Tests CONFIGURATIONS Debug RUNTIME DESTINATION bin/debug)
  install(TARGETS test_vault network_test_vault COMPONENT Tests CONFIGURATIONS Release RUNTIME DESTINATION bin)
endif()
