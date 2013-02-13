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

#ifndef MAIDSAFE_VAULT_PARAMETERS_H_
#define MAIDSAFE_VAULT_PARAMETERS_H_

#include <cstddef>


namespace maidsafe {

namespace vault {

namespace detail {

struct Parameters {
 public:
  // Min % returned by routing.network_status() to consider this node still online.
  static const int kMinNetworkHealth;
  // Max number of recent entries in account classes.
  static size_t max_recent_data_list_size;

  static void set_file_element_count_limits(size_t max_file_element_count,
                                            size_t min_file_element_count);
  static size_t max_file_element_count();
  static size_t min_file_element_count();
 private:
  Parameters();
  ~Parameters();
  Parameters(const Parameters&);
  Parameters& operator=(const Parameters&);
  Parameters(const Parameters&&);
  Parameters& operator=(Parameters&&);

  // Max count of elements allowed in each account file
  static size_t max_file_element_count_;
  // Min count of elements allowed in each account file
  static size_t min_file_element_count_;
};

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PARAMETERS_H_
