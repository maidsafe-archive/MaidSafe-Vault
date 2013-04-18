#ifndef MAIDSAFE_VAULT_ACCOUNT_DB_H_
#define MAIDSAFE_VAULT_ACCOUNT_DB_H_

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/vault/db.h"


namespace maidsafe {
namespace vault {

class AccountDb {
 public:
  explicit AccountDb(Db& db);
  ~AccountDb();
  void Put(const Db::KVPair& key_value_pair);
  void Delete(const DataNameVariant& key);
  NonEmptyString Get(const DataNameVariant& key);
  std::vector<Db::KVPair> Get();

 private:
  Db& db_;
  uint32_t account_id_;
};

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_DB_H_
