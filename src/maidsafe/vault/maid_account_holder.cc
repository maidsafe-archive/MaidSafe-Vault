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

#include "maidsafe/vault/maid_account_holder.h"

namespace maidsafe {

namespace vault {

MaidAccountHolder::MaidAccountHolder(const passport::Pmid& pmid,
                                     routing::Routing& routing,
                                     const boost::filesystem::path& vault_root_dir)
  : routing_(routing),
    kRootDir_(vault_root_dir / "maids"),
    nfs_(routing, pmid),
    maid_accounts_(),
    public_key_getter_(routing, std::vector<passport::PublicPmid>()) {
  boost::filesystem::exists(kRootDir_) || boost::filesystem::create_directory(kRootDir_);

  boost::filesystem::directory_iterator end_iter;
  for (boost::filesystem::directory_iterator dir_iter(kRootDir_);
      dir_iter != end_iter ; ++dir_iter) {
    if (boost::filesystem::is_regular_file(dir_iter->status())) {
      std::string account_content;
      if (ReadFile(*dir_iter, &account_content)) {
        maidsafe::nfs::MaidAccount maid_account;
        maid_account.Parse(NonEmptyString(account_content));
        assert(Identity(dir_iter->path().string()) == maid_account.maid_id);
        maid_accounts_.push_back(maid_account);
      }
    }
  }
}

void MaidAccountHolder::Serialise() {
  for (auto& account : maid_accounts_)
    WriteFile(kRootDir_ / account.maid_id.string(), account.Serialise().string());
}

void MaidAccountHolder::Serialise(const passport::Maid& maid) {
  auto itr = maid_accounts_.begin();
  while (itr != maid_accounts_.end()) {
    if ((*itr).maid_id.string() == maid.name().data.string()) {
      WriteFile(kRootDir_ / (*itr).maid_id.string(), (*itr).Serialise().string());
      break;
    }
    ++itr;
  }
}

void MaidAccountHolder::RemoveAccount(const passport::Maid& maid) {
  auto itr = maid_accounts_.begin();
  while (itr != maid_accounts_.end()) {
    if ((*itr).maid_id.string() == maid.name().data.string()) {
      boost::filesystem::remove(kRootDir_ / (*itr).maid_id.string());
      break;
    }
    ++itr;
  }
}

void MaidAccountHolder::Serialise(const passport::Pmid& /*pmid*/) {
}

bool MaidAccountHolder::HandleNewComer(const passport::PublicMaid& p_maid) {
  // TODO(Team): get public key / registration tokens
  //       validate tokens
  //       populate MaidAccout
  maidsafe::nfs::MaidAccount maid_account(p_maid.name().data);
  return WriteFile(kRootDir_ / maid_account.maid_id.string(), maid_account.Serialise().string());
}


MaidAccountHolder::~MaidAccountHolder() {
}

void MaidAccountHolder::OnCloseNodeReplaced(
    const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {
}

}  // namespace vault

}  // namespace maidsafe
