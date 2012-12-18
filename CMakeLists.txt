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

include(../../cmake_modules/maidsafe_standard_setup.cmake)

include_directories(${common_SOURCE_DIR}/include)
include_directories(${private_SOURCE_DIR}/include)
include_directories(${nfs_SOURCE_DIR}/include)
include_directories(${routing_SOURCE_DIR}/include)


#==================================================================================================#
# Set up all files as GLOBs                                                                        #
#==================================================================================================#
set(VaultSourcesRootDir ${PROJECT_SOURCE_DIR}/src/maidsafe/vault)
file(GLOB VaultSources ${VaultSourcesRootDir}/*.cc)
file(GLOB VaultHeaders ${VaultSourcesRootDir}/*.h)
set(VaultAllFiles ${VaultSources} ${VaultHeaders})
set(StyleAllFiles ${VaultAllFiles})
source_group("Vault Source Files" FILES ${VaultSources})
source_group("Vault Header Files" FILES ${VaultHeaders})
list(REMOVE_ITEM VaultAllFiles ${VaultSourcesRootDir}/vault_main.cc)

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

#target_link_libraries(maidsafe_vault maidsafe_common maidsafe_private maidsafe_utilities)
target_link_libraries(maidsafe_vault maidsafe_common maidsafe_private)
target_link_libraries(lifestuff_vault maidsafe_vault)

if(MAIDSAFE_TESTING)
  ms_add_executable(TESTvault "Tests/Vault" ${VaultTestsAllFiles})
  target_link_libraries(TESTvault maidsafe_vault)
  ms_add_executable(vault_key_helper Tools ${VaultSourcesRootDir}/tests/key_helper.cc)
endif()

rename_outdated_built_exes()


#==================================================================================================#
# Set compiler and linker flags                                                                    #
#==================================================================================================#
include(maidsafe_standard_flags)


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
