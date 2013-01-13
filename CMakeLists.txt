#==================================================================================================#
#                                                                                                  #
#  Copyright (c) 2012 MaidSafe.net limited                                                         #
#                                                                                                  #
#  The following source code is property of MaidSafe.net limited and is not meant for external     #
#  use.  The use of this code is governed by the license file licence.txt found in the root        #
#  directory of this project and also on www.maidsafe.net.                                         #
#                                                                                                  #
#  You are not free to copy, amend or otherwise use this source code without the explicit written  #
#  permission of the board of directors of MaidSafe.net.                                           #
#                                                                                                  #
#==================================================================================================#


set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)
project(vault)

set(PROTO_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
file(GLOB_RECURSE PROTO_FILES RELATIVE ${PROTO_SOURCE_DIR} "${PROTO_SOURCE_DIR}/maidsafe/vault/*.proto")
include(../../cmake_modules/maidsafe_standard_setup.cmake)

include_directories(${common_SOURCE_DIR}/include)
include_directories(${passport_SOURCE_DIR}/include)
include_directories(${private_SOURCE_DIR}/include)
include_directories(${nfs_SOURCE_DIR}/include)
include_directories(${routing_SOURCE_DIR}/include)
include_directories(${rudp_SOURCE_DIR}/include)


#==================================================================================================#
# Set up all files as GLOBs                                                                        #
#==================================================================================================#
set(VaultSourcesRootDir ${PROJECT_SOURCE_DIR}/src/maidsafe/vault)
file(GLOB_RECURSE VaultProtos ${VaultSourcesRootDir}/*.proto)
file(GLOB_RECURSE VaultProtoSources ${VaultSourcesRootDir}/*.pb.cc)
file(GLOB_RECURSE VaultProtoHeaders ${VaultSourcesRootDir}/*pb.h)
set(VaultProtos ${VaultProtos} ${VaultProtoSources} ${VaultProtoHeaders})
source_group("Vault Proto Files" FILES ${VaultProtos})

file(GLOB VaultSources ${VaultSourcesRootDir}/*.cc)
file(GLOB VaultHeaders ${VaultSourcesRootDir}/*.h)
list(REMOVE_ITEM VaultSources ${VaultProtos})
list(REMOVE_ITEM VaultHeaders ${VaultProtos})
source_group("Vault Source Files" FILES ${VaultSources})
source_group("Vault Header Files" FILES ${VaultHeaders})
list(REMOVE_ITEM VaultSources ${VaultSourcesRootDir}/vault_main.cc)

file(GLOB VaultDataHolderSources ${VaultSourcesRootDir}/data_holder/*.cc)
file(GLOB VaultDataHolderHeaders ${VaultSourcesRootDir}/data_holder/*.h)
list(REMOVE_ITEM VaultDataHolderSources ${VaultProtos})
list(REMOVE_ITEM VaultDataHolderHeaders ${VaultProtos})
source_group("Data Holder Source Files" FILES ${VaultDataHolderSources})
source_group("Data Holder Header Files" FILES ${VaultDataHolderHeaders})

file(GLOB VaultMaidAccountHolderSources ${VaultSourcesRootDir}/maid_account_holder/*.cc)
file(GLOB VaultMaidAccountHolderHeaders ${VaultSourcesRootDir}/maid_account_holder/*.h)
list(REMOVE_ITEM VaultMaidAccountHolderSources ${VaultProtos})
list(REMOVE_ITEM VaultMaidAccountHolderHeaders ${VaultProtos})
source_group("MAID Account Holder Source Files" FILES ${VaultMaidAccountHolderSources})
source_group("MAID Account Holder Header Files" FILES ${VaultMaidAccountHolderHeaders})

file(GLOB VaultMetadataManagerSources ${VaultSourcesRootDir}/metadata_manager/*.cc)
file(GLOB VaultMetadataManagerHeaders ${VaultSourcesRootDir}/metadata_manager/*.h)
list(REMOVE_ITEM VaultMetadataManagerSources ${VaultProtos})
list(REMOVE_ITEM VaultMetadataManagerHeaders ${VaultProtos})
source_group("Metadata Manager Source Files" FILES ${VaultMetadataManagerSources})
source_group("Metadata Manager Header Files" FILES ${VaultMetadataManagerHeaders})

file(GLOB VaultMpidAccountHolderSources ${VaultSourcesRootDir}/mpid_account_holder/*.cc)
file(GLOB VaultMpidAccountHolderHeaders ${VaultSourcesRootDir}/mpid_account_holder/*.h)
list(REMOVE_ITEM VaultMpidAccountHolderSources ${VaultProtos})
list(REMOVE_ITEM VaultMpidAccountHolderHeaders ${VaultProtos})
source_group("MPID Account Holder Source Files" FILES ${VaultMpidAccountHolderSources})
source_group("MPID Account Holder Header Files" FILES ${VaultMpidAccountHolderHeaders})

file(GLOB VaultPmidAccountHolderSources ${VaultSourcesRootDir}/pmid_account_holder/*.cc)
file(GLOB VaultPmidAccountHolderHeaders ${VaultSourcesRootDir}/pmid_account_holder/*.h)
list(REMOVE_ITEM VaultPmidAccountHolderSources ${VaultProtos})
list(REMOVE_ITEM VaultPmidAccountHolderHeaders ${VaultProtos})
source_group("PMID Account Holder Source Files" FILES ${VaultPmidAccountHolderSources})
source_group("PMID Account Holder Header Files" FILES ${VaultPmidAccountHolderHeaders})

set(VaultAllFiles ${VaultSources} ${VaultHeaders}
                  ${VaultDataHolderSources} ${VaultDataHolderHeaders}
                  ${VaultMaidAccountHolderSources} ${VaultMaidAccountHolderHeaders}
                  ${VaultMetadataManagerSources} ${VaultMetadataManagerHeaders}
                  ${VaultMpidAccountHolderSources} ${VaultMpidAccountHolderHeaders}
                  ${VaultPmidAccountHolderSources} ${VaultPmidAccountHolderHeaders})
set(StyleAllFiles ${VaultAllFiles})
set(VaultAllFiles ${VaultAllFiles} ${VaultProtos})

file(GLOB VaultTestsSources ${VaultSourcesRootDir}/tests/*.cc)
file(GLOB VaultTestsHeaders ${VaultSourcesRootDir}/tests/*.h)
set(VaultTestsAllFiles ${VaultTestsSources} ${VaultTestsHeaders})
set(StyleAllFiles ${StyleAllFiles} ${VaultTestsAllFiles})
source_group("Tests Source Files" FILES ${VaultTestsSources})
source_group("Tests Header Files" FILES ${VaultTestsHeaders})
list(REMOVE_ITEM VaultTestsAllFiles ${VaultSourcesRootDir}/tests/key_helper.cc)


#==================================================================================================#
# Define MaidSafe libraries and executables                                                        #
#==================================================================================================#
ms_add_static_library(vault ${VaultAllFiles})
ms_add_executable(lifestuff_vault Production ${VaultSourcesRootDir}/vault_main.cc)

target_link_libraries(maidsafe_vault maidsafe_nfs maidsafe_routing maidsafe_common maidsafe_private)
target_link_libraries(lifestuff_vault maidsafe_vault maidsafe_routing maidsafe_lifestuff_manager)

if(MAIDSAFE_TESTING)
  ms_add_executable(TESTvault "Tests/Vault" ${VaultTestsAllFiles})
  target_link_libraries(TESTvault maidsafe_vault)
  ms_add_executable(vault_key_helper Tools ${VaultSourcesRootDir}/tests/key_helper.cc)
  target_link_libraries(vault_key_helper maidsafe_routing maidsafe_nfs maidsafe_private)
endif()

rename_outdated_built_exes()


#==================================================================================================#
# Set compiler and linker flags                                                                    #
#==================================================================================================#
include(maidsafe_standard_flags)

ms_disable_warnings(${VaultProtoSources})

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-effc++")
elseif(MSVC AND MAIDSAFE_TESTING)
  set_target_properties(TESTvault PROPERTIES COMPILE_FLAGS "/bigobj")
endif()

#==================================================================================================#
# Tests                                                                                            #
#==================================================================================================#
if(MAIDSAFE_TESTING)
  add_style_test(${StyleAllFiles})
  add_gtests(TESTvault)
#   label_as_critical_tests(IdentityPacketsTest.BEH_Full
#                           IdentityPacketsTest.BEH_ChangeDetails
#                           PassportTest.BEH_SigningPackets
#                           PassportTest.BEH_IdentityPackets
#                           PassportTest.BEH_ChangingIdentityPackets
#                           PassportTest.BEH_MoveMaidsafeInbox
#                           PassportTest.BEH_SerialiseParse
#                           )
  add_project_experimental()
  test_summary_output()
endif()
