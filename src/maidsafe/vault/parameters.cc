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

#include "maidsafe/vault/parameters.h"

#include <exception>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"


namespace maidsafe {

namespace vault {

namespace detail {

const int Parameters::kMinNetworkHealth(12);
size_t Parameters::max_recent_data_list_size(1000);
int Parameters::max_file_element_count(10000);

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
