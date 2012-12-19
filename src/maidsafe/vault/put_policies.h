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

#ifndef MAIDSAFE_VAULT_PUT_POLICIES_H_
#define MAIDSAFE_VAULT_PUT_POLICIES_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/thread/mutex.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/rsa.h"

namespace maidsafe {

namespace vault {

template <typename T>
class PutToMetaDataManager {
 public:
  static void  PutPolicy<>(name, callback, routing, fob) {
  }
  static void  PutPolicy<MutableData>(name, callback, routing, fob) {

  }

 protected:
  ~PutToMetaDataManager() {}
};

template <typename T>
class PutToPmidAccountHolder {
 public:
  static void  PutPolicy<>(name, callback, routing, fob) {
  }
  static void  PutPolicy<MutableData>(name, callback, routing, fob) {

  }

 protected:
  ~PutToPmidAccountHolder() {}
};

template <typename T>
class PutToDataHolder {
 public:
  static void  PutPolicy<>(name, callback, routing, fob) {
  }
  static void  PutPolicy<MutableData>(name, callback, routing, fob) {

  }

 protected:
  ~PutToDataHolder() {}
};

template <typename T>
class NoPut {
 public:
  static void Put<>(name, callback, routing, fob) {
  }
 protected:
  ~NoPut() {}
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PUT_POLICIES_H_
