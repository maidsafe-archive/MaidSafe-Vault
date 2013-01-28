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

#include "maidsafe/common/log.h"

namespace maidsafe {

namespace vault {

namespace detail {

const int Parameters::kMinNetworkHealth(12);
size_t Parameters::max_recent_data_list_size(1000);
size_t Parameters::max_file_element_count_ = 10000;
size_t Parameters::min_file_element_count_ = 5000;

void Parameters::set_file_element_count_limits(size_t max_file_element_count,
                                               size_t min_file_element_count) {
  if (min_file_element_count < max_file_element_count / 2) {
    LOG(kError) << "min_file_element_count must be at least half of max_file_element_count";
    throw std::exception();
  }
  max_file_element_count_ = max_file_element_count;
  min_file_element_count_ = min_file_element_count;
}

size_t Parameters::max_file_element_count() { return max_file_element_count_; }

size_t Parameters::min_file_element_count() { return min_file_element_count_; }

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
