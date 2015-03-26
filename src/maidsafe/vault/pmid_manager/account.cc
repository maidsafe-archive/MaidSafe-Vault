/*  Copyright 2015 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/pmid_manager/account.h"

#include "maidsafe/common/utils.h"

#include "maidsafe/common/serialisation/serialisation.h"

namespace maidsafe {
namespace vault {

PmidManagerAccount::PmidManagerAccount()
    : stored_total_size(0), lost_total_size(0), offered_space(0) {}

PmidManagerAccount::PmidManagerAccount(const uint64_t& stored_total_size_in,
                                       const uint64_t& lost_total_size_in,
                                       const uint64_t& offered_space_in)
    : stored_total_size(stored_total_size_in), lost_total_size(lost_total_size_in),
      offered_space(offered_space_in) {}

PmidManagerAccount::PmidManagerAccount(const std::string &serialised_account)
    : stored_total_size(0), lost_total_size(0), offered_space(0) {
  maidsafe::ConvertFromString(serialised_account,
                              offered_space, stored_total_size, lost_total_size);
}

PmidManagerAccount::PmidManagerAccount(const PmidManagerAccount& other)
    : stored_total_size(other.stored_total_size),
      lost_total_size(other.lost_total_size),
      offered_space(other.offered_space) {}

PmidManagerAccount::PmidManagerAccount(PmidManagerAccount&& other)
    : stored_total_size(std::move(other.stored_total_size)),
      lost_total_size(std::move(other.lost_total_size)),
      offered_space(std::move(other.offered_space)) {}

PmidManagerAccount& PmidManagerAccount::operator=(PmidManagerAccount other) {
  using std::swap;
  swap(stored_total_size, other.stored_total_size);
  swap(lost_total_size, other.lost_total_size);
  swap(offered_space, other.offered_space);
  return *this;
}

void PmidManagerAccount::PutData(uint64_t size) {
  stored_total_size += size;
}

void PmidManagerAccount::DeleteData(uint64_t size) {
  if (stored_total_size < size) {
    LOG(kError) << "invalid stored_total_size " << stored_total_size;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_argument));
  }
  stored_total_size -= size;
}

void PmidManagerAccount::HandleLostData(uint64_t size) {
  DeleteData(size);
  lost_total_size += size;
}

void PmidManagerAccount::HandleFailure(uint64_t size) {
  HandleLostData(size);
}

void PmidManagerAccount::SetAvailableSize(const int64_t& available_size) {
  offered_space = available_size;
}

void PmidManagerAccount::UpdateAccount(int64_t diff_size) {
  stored_total_size =
      (static_cast<int64_t>(stored_total_size) < diff_size) ? 0 : (stored_total_size - diff_size);
  lost_total_size += diff_size;
}

std::string PmidManagerAccount::serialise() const {
  return maidsafe::ConvertToString(offered_space, lost_total_size, stored_total_size);
}

bool operator==(const PmidManagerAccount& lhs, const PmidManagerAccount& rhs) {
  return lhs.stored_total_size == rhs.stored_total_size &&
         lhs.lost_total_size == rhs.lost_total_size &&
         lhs.offered_space == rhs.offered_space;
}

}  // namespace vault

}  // namespace maidsafe
