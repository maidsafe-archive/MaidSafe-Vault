///*  Copyright 2013 MaidSafe.net limited

//    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
//    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
//    licence you accepted on initial access to the Software (the "Licences").

//    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
//    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
//    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
//    available at: http://www.maidsafe.net/licenses

//    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
//    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
//    OF ANY KIND, either express or implied.

//    See the Licences for the specific language governing permissions and limitations relating to
//    use of the MaidSafe Software.                                                                 */


//#include "maidsafe/common/test.h"
//#include "maidsafe/common/utils.h"

//#include "maidsafe/data_types/immutable_data.h"
//#include "maidsafe/data_types/data_name_variant.h"

//#include "maidsafe/vault/utils-inl.h"


//namespace maidsafe {

//namespace vault {

//namespace test {

//TEST(DbKeyTest, BEH_Serialise) {
//  DataNameVariant name(ImmutableData::Name(Identity(
//      RandomString(crypto::SHA512::DIGESTSIZE))));
//  DbKey db_key(name);
//  EXPECT_TRUE(db_key.name() == name);

//  // Simulation a serialisation
//  static GetTagValueAndIdentityVisitor visitor;
//  auto result(boost::apply_visitor(visitor, name));
//  std::string simulated = std::string(result.second.string() +
//            maidsafe::vault::detail::ToFixedWidthString<1>(static_cast<uint32_t>(result.first)));

//  EXPECT_EQ(simulated, db_key.Serialise());
//}

//TEST(DbKeyTest, BEH_All) {
//  DataNameVariant name(ImmutableData::Name(Identity(
//      RandomString(crypto::SHA512::DIGESTSIZE))));
//  DbKey db_key;
//  db_key = DbKey(name);
//  EXPECT_TRUE(db_key.name() == name);

//  auto temp = name;
//  DbKey db_key_copy1(db_key);
//  EXPECT_TRUE(db_key == db_key_copy1);
//  EXPECT_FALSE(db_key != db_key_copy1);
//  EXPECT_FALSE(db_key < db_key_copy1);
//  EXPECT_FALSE(db_key > db_key_copy1);
//  EXPECT_TRUE(db_key <= db_key_copy1);
//  EXPECT_TRUE(db_key >= db_key_copy1);

//  DbKey db_key_copy2(std::move(db_key_copy1));
//  EXPECT_EQ(db_key, db_key_copy2);

//  DbKey db_key_copy3;
//  db_key_copy3 = std::move(db_key_copy2);
//  EXPECT_EQ(db_key, db_key_copy3);

//  auto serialised_db_key(db_key.Serialise());
//  DbKey parsed_db_key(serialised_db_key);
//  EXPECT_EQ(db_key, parsed_db_key);
//}

//}  // namespace test

//}  // namespace vault

//}  // namespace maidsafe
