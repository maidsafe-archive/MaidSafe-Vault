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

file(GLOB VaultProtos ${VaultSourcesRootDir}/*.proto)
file(GLOB VaultProtoSources ${VaultSourcesRootDir}/*.pb.cc)
file(GLOB VaultProtoHeaders ${VaultSourcesRootDir}/*pb.h)
set(VaultProtos ${VaultProtos} ${VaultProtoSources} ${VaultProtoHeaders})
file(GLOB VaultSources ${VaultSourcesRootDir}/*.cc)
file(GLOB VaultHeaders ${VaultSourcesRootDir}/*.h)
list(REMOVE_ITEM VaultSources ${VaultProtos})
list(REMOVE_ITEM VaultHeaders ${VaultProtos})
source_group("Vault Source Files" FILES ${VaultSources})
source_group("Vault Header Files" FILES ${VaultHeaders})
source_group("Vault Proto Files" FILES ${VaultProtos})
list(REMOVE_ITEM VaultSources ${VaultSourcesRootDir}/vault_main.cc)

file(GLOB DataHolderProtos ${VaultSourcesRootDir}/data_holder/*.proto)
file(GLOB DataHolderProtoSources ${VaultSourcesRootDir}/data_holder/*.pb.cc)
file(GLOB DataHolderProtoHeaders ${VaultSourcesRootDir}/data_holder/*pb.h)
set(DataHolderProtos ${DataHolderProtos} ${DataHolderProtoSources} ${DataHolderProtoHeaders})
file(GLOB DataHolderSources ${VaultSourcesRootDir}/data_holder/*.cc)
file(GLOB DataHolderHeaders ${VaultSourcesRootDir}/data_holder/*.h)
# list(REMOVE_ITEM DataHolderSources ${DataHolderProtos})
# list(REMOVE_ITEM DataHolderHeaders ${DataHolderProtos})
source_group("Data Holder\\Source Files" FILES ${DataHolderSources})
source_group("Data Holder\\Header Files" FILES ${DataHolderHeaders})
source_group("Data Holder\\Proto Files" FILES ${DataHolderProtos})

file(GLOB MaidAccountHolderProtos ${VaultSourcesRootDir}/maid_account_holder/*.proto)
file(GLOB MaidAccountHolderProtoSources ${VaultSourcesRootDir}/maid_account_holder/*.pb.cc)
file(GLOB MaidAccountHolderProtoHeaders ${VaultSourcesRootDir}/maid_account_holder/*pb.h)
set(MaidAccountHolderProtos ${MaidAccountHolderProtos} ${MaidAccountHolderProtoSources} ${MaidAccountHolderProtoHeaders})
file(GLOB MaidAccountHolderSources ${VaultSourcesRootDir}/maid_account_holder/*.cc)
file(GLOB MaidAccountHolderHeaders ${VaultSourcesRootDir}/maid_account_holder/*.h)
list(REMOVE_ITEM MaidAccountHolderSources ${MaidAccountHolderProtos})
list(REMOVE_ITEM MaidAccountHolderHeaders ${MaidAccountHolderProtos})
source_group("Maid Account Holder\\Source Files" FILES ${MaidAccountHolderSources})
source_group("Maid Account Holder\\Header Files" FILES ${MaidAccountHolderHeaders})
source_group("Maid Account Holder\\Proto Files" FILES ${MaidAccountHolderProtos})

file(GLOB MetadataManagerProtos ${VaultSourcesRootDir}/metadata_manager/*.proto)
file(GLOB MetadataManagerProtoSources ${VaultSourcesRootDir}/metadata_manager/*.pb.cc)
file(GLOB MetadataManagerProtoHeaders ${VaultSourcesRootDir}/metadata_manager/*pb.h)
set(MetadataManagerProtos ${MetadataManagerProtos} ${MetadataManagerProtoSources} ${MetadataManagerProtoHeaders})
file(GLOB MetadataManagerSources ${VaultSourcesRootDir}/metadata_manager/*.cc)
file(GLOB MetadataManagerHeaders ${VaultSourcesRootDir}/metadata_manager/*.h)
list(REMOVE_ITEM MetadataManagerSources ${MetadataManagerProtos})
list(REMOVE_ITEM MetadataManagerHeaders ${MetadataManagerProtos})
source_group("Metadata Manager\\Source Files" FILES ${MetadataManagerSources})
source_group("Metadata Manager\\Header Files" FILES ${MetadataManagerHeaders})
source_group("Metadata Manager\\Proto Files" FILES ${MetadataManagerProtos})

file(GLOB MpidAccountHolderProtos ${VaultSourcesRootDir}/mpid_account_holder/*.proto)
file(GLOB MpidAccountHolderProtoSources ${VaultSourcesRootDir}/mpid_account_holder/*.pb.cc)
file(GLOB MpidAccountHolderProtoHeaders ${VaultSourcesRootDir}/mpid_account_holder/*pb.h)
set(MpidAccountHolderProtos ${MpidAccountHolderProtos} ${MpidAccountHolderProtoSources} ${MpidAccountHolderProtoHeaders})
file(GLOB MpidAccountHolderSources ${VaultSourcesRootDir}/mpid_account_holder/*.cc)
file(GLOB MpidAccountHolderHeaders ${VaultSourcesRootDir}/mpid_account_holder/*.h)
# list(REMOVE_ITEM MpidAccountHolderSources ${MpidAccountHolderProtos})
# list(REMOVE_ITEM MpidAccountHolderHeaders ${MpidAccountHolderProtos})
source_group("Mpid Account Holder\\Source Files" FILES ${MpidAccountHolderSources})
source_group("Mpid Account Holder\\Header Files" FILES ${MpidAccountHolderHeaders})
source_group("Mpid Account Holder\\Proto Files" FILES ${MpidAccountHolderProtos})

file(GLOB PmidAccountHolderProtos ${VaultSourcesRootDir}/pmid_account_holder/*.proto)
file(GLOB PmidAccountHolderProtoSources ${VaultSourcesRootDir}/pmid_account_holder/*.pb.cc)
file(GLOB PmidAccountHolderProtoHeaders ${VaultSourcesRootDir}/pmid_account_holder/*pb.h)
set(PmidAccountHolderProtos ${PmidAccountHolderProtos} ${PmidAccountHolderProtoSources} ${PmidAccountHolderProtoHeaders})
file(GLOB PmidAccountHolderSources ${VaultSourcesRootDir}/pmid_account_holder/*.cc)
file(GLOB PmidAccountHolderHeaders ${VaultSourcesRootDir}/pmid_account_holder/*.h)
list(REMOVE_ITEM PmidAccountHolderSources ${PmidAccountHolderProtos})
list(REMOVE_ITEM PmidAccountHolderHeaders ${PmidAccountHolderProtos})
source_group("Pmid Account Holder\\Source Files" FILES ${PmidAccountHolderSources})
source_group("Pmid Account Holder\\Header Files" FILES ${PmidAccountHolderHeaders})
source_group("Pmid Account Holder\\Proto Files" FILES ${PmidAccountHolderProtos})

set(VaultAllFiles ${VaultSources} ${VaultHeaders}
                  ${DataHolderSources} ${DataHolderHeaders}
                  ${MaidAccountHolderSources} ${MaidAccountHolderHeaders}
                  ${MetadataManagerSources} ${MetadataManagerHeaders}
                  ${MpidAccountHolderSources} ${MpidAccountHolderHeaders}
                  ${PmidAccountHolderSources} ${PmidAccountHolderHeaders})
set(StyleAllFiles ${VaultAllFiles})
set(VaultAllFiles ${VaultAllFiles}
                  ${VaultProtos}
                  ${DataHolderProtos}
                  ${MaidAccountHolderProtos}
                  ${MetadataManagerProtos}
                  ${MpidAccountHolderProtos}
                  ${PmidAccountHolderProtos})

file(GLOB VaultTestsSources ${VaultSourcesRootDir}/tests/*.cc)
file(GLOB VaultTestsHeaders ${VaultSourcesRootDir}/tests/*.h)
set(VaultTestsAllFiles ${VaultTestsSources} ${VaultTestsHeaders})
set(StyleAllFiles ${StyleAllFiles} ${VaultTestsAllFiles})
source_group("Tests Source Files" FILES ${VaultTestsSources})
source_group("Tests Header Files" FILES ${VaultTestsHeaders})

file(GLOB VaultToolsSources ${VaultSourcesRootDir}/tools/*.cc)
file(GLOB VaultToolsHeaders ${VaultSourcesRootDir}/tools/*.h)
set(VaultToolsAllFiles ${VaultToolsSources} ${VaultToolsHeaders})
set(StyleAllFiles ${StyleAllFiles} ${VaultToolsAllFiles})
source_group("Tools Source Files" FILES ${VaultToolsSources})
source_group("Tools Header Files" FILES ${VaultToolsHeaders})


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
  ms_add_executable(vault_key_helper Tools ${VaultToolsAllFiles})
  target_link_libraries(vault_key_helper maidsafe_routing maidsafe_nfs maidsafe_private)
endif()

rename_outdated_built_exes()


#==================================================================================================#
# Set compiler and linker flags                                                                    #
#==================================================================================================#
include(maidsafe_standard_flags)

ms_disable_warnings(${VaultProtoSources} ${DataHolderProtoSources} ${MaidAccountHolderProtoSources}
                    ${MetadataManagerProtoSources} ${MpidAccountHolderProtoSources} ${PmidAccountHolderProtoSources})

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-effc++")
elseif(MSVC AND MAIDSAFE_TESTING)
  set_target_properties(maidsafe_vault TESTvault PROPERTIES COMPILE_FLAGS "/bigobj")
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
