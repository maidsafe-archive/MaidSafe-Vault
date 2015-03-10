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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_MESSAGES_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_MESSAGES_H_

#include "maidsafe/common/config.h"
#include "maidsafe/common/bounded_string.h"
#include "maidsafe/passport/types.h"

namespace maidsafe {

namespace vault {

const unsigned int kMaxHeaderSize = 128;
const unsigned int kMaxBodySize = 1024 * 1024;
const unsigned int kIdSize = 64;
using  MessageIdType = detail::BoundedString<kIdSize, kIdSize>;
using  MessageHeaderType = detail::BoundedString<0, kMaxHeaderSize>;
using  MessageBodyType = detail::BoundedString<0, kMaxBodySize>;

// ================================= MpidMessageBase =============================================

struct MpidMessageBase {
  MpidMessageBase();
  MpidMessageBase(const passport::PublicMpid::Name& sender_in,
                  const passport::PublicMpid::Name& receiver_in, int32_t id_in,
                  int32_t parent_id_in, const MessageHeaderType& signed_header_in);
  explicit MpidMessageBase(const std::string& serialised_copy);
  MpidMessageBase(const MpidMessageBase& other);
  MpidMessageBase(MpidMessageBase&& other);
  MpidMessageBase& operator=(MpidMessageBase other);

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(sender, receiver, id, parent_id, signed_header);
  }

  passport::PublicMpid::Name sender, receiver;
  int32_t id, parent_id;
  MessageHeaderType signed_header;
};

bool operator==(const MpidMessageBase& lhs, const MpidMessageBase& rhs);
void swap(MpidMessageBase& lhs, MpidMessageBase& rhs) MAIDSAFE_NOEXCEPT;

// ================================= MpidAlert =============================================

struct MpidAlert {
  MpidAlert();
  MpidAlert(const MpidMessageBase& base_in, const MessageIdType& message_id_in);
  explicit MpidAlert(const std::string& serialised_copy);
  MpidAlert(const MpidAlert& other);
  MpidAlert(MpidAlert&& other);
  MpidAlert& operator=(MpidAlert other);

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(message_id, base);
  }

  MpidMessageBase base;
  MessageIdType message_id;
};

bool operator==(const MpidAlert& lhs, const MpidAlert& rhs);
void swap(MpidAlert& lhs, MpidAlert& rhs) MAIDSAFE_NOEXCEPT;

// ================================= MpidMessage ==================================================

struct MpidMessage {
  MpidMessage(const MpidMessageBase& base_in, MessageBodyType& signed_body_in);
  explicit MpidMessage(const std::string& serialised_copy);
  MpidMessage(const MpidMessage& other);
  MpidMessage(MpidMessage&& other);
  MpidMessage& operator=(MpidMessage other);

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(base, signed_body);
  }

  MpidMessageBase base;
  MessageBodyType signed_body;
};

bool operator==(const MpidMessage& lhs, const MpidMessage& rhs);
void swap(MpidMessage& lhs, MpidMessage& rhs) MAIDSAFE_NOEXCEPT;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_MESSAGES_H_

