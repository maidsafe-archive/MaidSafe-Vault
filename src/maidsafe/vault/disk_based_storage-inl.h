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

#ifndef MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_
#define MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_

#include <string>


namespace maidsafe {

namespace vault {

template<typename Data>
size_t DiskBasedStorage::GetElementCount(const typename Data::name_type& name) const {
  FileDetails::ElementNameSubstr name_substr(name->string().substr(0, FileDetails::kSubstrSize));
  auto lower(GetFileIdsLowerBound(name_substr));
  if (lower == std::end(file_ids_))
    return 0;
  auto file(ParseFile(*lower, false).first);

  int instances(0);
  for (int i(0); i != file.element_size(); ++i) {
    if (file.element(i).name() == name->string() &&
        file.element(i).type() == static_cast<int32_t>(Data::type_enum_value())) {
      ++instances;
    } else if (instances) {
      break;
    }
  }
  return instances;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_
