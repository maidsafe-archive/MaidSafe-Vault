/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_ACTIVE_FILE_HANDLER_H_
#define MAIDSAFE_VAULT_ACTIVE_FILE_HANDLER_H_

#include "maidsafe/common/active.h"
#include "maidsafe/common/utils.h"

namespace maidsafe {

namespace vault {

template <typename C, typename E>
class ActiveFileHandler {
 public:
  ActiveFileHandler(const boost::filesystem::path& base_path,
                    const boost::filesystem::path& file_name);

  void Write(Identity name, const E& container_element);
  void Read(Identity name, E& container_element);
  void Delete(Identity name, E container_element);

 private:
  Active active_;
  int current_element_count_, current_file_;
  const boost::filesystem::path file_name_;
  boost::filesystem::path base_path_, curent_file_path_;

  void CheckForExistingFiles();
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/active_file_handler-inl.h"

#endif  // MAIDSAFE_VAULT_ACTIVE_FILE_HANDLER_H_
