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

#ifndef MAIDSAFE_VAULT_GET_POLICIES_H_
#define MAIDSAFE_VAULT_GET_POLICIES_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"


namespace maidsafe {

namespace vault {
// MetaDataManager will need this if the get request comes that far
class GetFromDataHolder {
 public:
template <typename T>
  static void Get(typename T::name_type /* name */) {
  }
 protected:
  ~GetFromDataHolder() {}
};

class NoGet {
 public:
template <typename T>
  static void Get(typename T::name_type /* name */) {
  }
 protected:
  ~NoGet() {}
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GET_POLICIES_H_
