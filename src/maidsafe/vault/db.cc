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


#include "maidsafe/vault/db.h"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {

namespace vault {

std::unique_ptr<leveldb::DB> Db::leveldb_ = nullptr;
std::atomic<uint32_t> Db::last_account_id_(0);

Db::Db(const boost::filesystem::path& path) {
  if (!leveldb_) { //FIXME use call once
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = true; // FIXME  need to delete existing db on start
    leveldb::Status status = leveldb::DB::Open(options, path.string(), &db);
    leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
    assert(status.ok());
    assert(leveldb_);
  }
  account_id_ = ++last_account_id_;
}

}  // namespace vault

}  // namespace maidsafe
