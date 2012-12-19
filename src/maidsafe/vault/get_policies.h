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
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/thread/mutex.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/rsa.h"

#include "maidsafe/pd/client/node.h"

namespace maidsafe {

namespace vault {
// MetaDataManager will need this if the get request comes that far
template <typename T>
class GetFromDataHolder {
 public:
  static void  GetPolicy<>(name, callback, routing, fob) {
  }
 protected:
  ~GetFromDataHolder() {}
};

template <typename T>
class NoGet {
 public:
  static void Get<>(T::name_type name) {
  }
 protected:
  ~NoGet() {}
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GET_POLICIES_H_
