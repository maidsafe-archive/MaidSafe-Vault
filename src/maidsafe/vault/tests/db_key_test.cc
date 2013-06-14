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

#include "maidsafe/vault/db_key.h"

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/data_types/immutable_data.h"
#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/utils-inl.h"


namespace maidsafe {

namespace vault {

namespace test {

TEST(DbKeyTest, BEH_Serialise) {
  DataNameVariant name(ImmutableData::name_type(Identity(
      RandomString(crypto::SHA512::DIGESTSIZE))));
  DbKey db_key(name);
  EXPECT_TRUE(db_key.name() == name);

  // Simulation a serialisation
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, name));
  std::string simulated = std::string(result.second.string() +
            maidsafe::vault::detail::ToFixedWidthString<1>(static_cast<uint32_t>(result.first)));

  EXPECT_EQ(simulated, db_key.Serialise());
}

TEST(DbKeyTest, BEH_All) {
  DataNameVariant name(ImmutableData::name_type(Identity(
      RandomString(crypto::SHA512::DIGESTSIZE))));
  DbKey db_key;
  db_key = DbKey(name);
  EXPECT_TRUE(db_key.name() == name);

  auto temp = name;
  DbKey db_key_copy1(db_key);
  EXPECT_TRUE(db_key == db_key_copy1);
  EXPECT_FALSE(db_key != db_key_copy1);
  EXPECT_FALSE(db_key < db_key_copy1);
  EXPECT_FALSE(db_key > db_key_copy1);
  EXPECT_TRUE(db_key <= db_key_copy1);
  EXPECT_TRUE(db_key >= db_key_copy1);

  DbKey db_key_copy2(std::move(db_key_copy1));
  EXPECT_EQ(db_key, db_key_copy2);

  DbKey db_key_copy3;
  db_key_copy3 = std::move(db_key_copy2);
  EXPECT_EQ(db_key, db_key_copy3);

  auto serialised_db_key(db_key.Serialise());
  DbKey parsed_db_key(serialised_db_key);
  EXPECT_EQ(db_key, parsed_db_key);
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
