/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_KEY_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_KEY_H_

#include <iostream>

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/persona_id.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

class Db;
template<typename PersonaType>
class ManagerDb;

class StructuredDataKey {
  StructuredDataKey(const DataNameVariant& data_name_in, const Identity& originator_in);
  StructuredDataKey(const StructuredDataKey& other);
  StructuredDataKey(StructuredDataKey&& other);
  StructuredDataKey& operator=(StructuredDataKey other);
  DataNameVariant data_name() { return data_name_ ; }
  Identity originator() { return originator_; }
private:
  friend class Db;
  template<typename PersonaType> friend class ManagerDb;
  explicit StructuredDataKey(const std::string&);
  std::string Serialise() const;
  DataNameVariant data_name_;
  Identity originator_;
  static const int kPaddedWidth;
};

void swap(StructuredDataKey& lhs, StructuredDataKey& rhs) MAIDSAFE_NOEXCEPT;

bool operator==(const StructuredDataKey& lhs, const StructuredDataKey& rhs);

bool operator!=(const StructuredDataKey& lhs, const StructuredDataKey& rhs);

bool operator<(const StructuredDataKey& lhs, const StructuredDataKey& rhs);

bool operator>(const StructuredDataKey& lhs, const StructuredDataKey& rhs);

bool operator<=(const StructuredDataKey& lhs, const StructuredDataKey& rhs);

bool operator>=(const StructuredDataKey& lhs, const StructuredDataKey& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_KEY_H_
