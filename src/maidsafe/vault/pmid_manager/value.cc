/*  Copyright 2012 MaidSafe.net limited

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

#include "maidsafe/vault/pmid_manager/value.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"

namespace maidsafe {
namespace vault {

PmidManagerValue::PmidManagerValue()
   : data_elements_([](const ValueType& lhs, const ValueType& rhs) {
                      return (boost::apply_visitor(GetIdentityVisitor(), lhs.first)).string() <
                             (boost::apply_visitor(GetIdentityVisitor(), rhs.first)).string();
                    }) {}

PmidManagerValue::PmidManagerValue(const std::string& serialised_pmid_manager_value)
    : data_elements_() {
  protobuf::PmidManagerValue value_proto;
  if (!value_proto.ParseFromString(serialised_pmid_manager_value)) {
    LOG(kError) << "Failed to read or parse serialised pmid manager value.";
    ThrowError(CommonErrors::parsing_error);
  } else {
    for (auto index(0); index < value_proto.data_elements_size(); ++index) {
      data_elements_.insert(
          std::make_pair(
              GetDataNameVariant(
                  static_cast<DataTagValue>(value_proto.data_elements(index).data_type()),
                  Identity(value_proto.data_elements(index).data_name())),
              value_proto.data_elements(index).size()));
    }
  }
}

PmidManagerValue::PmidManagerValue(const PmidManagerValue& other)
    : data_elements_(other.data_elements_) {}

PmidManagerValue::PmidManagerValue(PmidManagerValue&& other)
    : data_elements_(std::move(other.data_elements_)) {}

PmidManagerValue& PmidManagerValue::operator=(PmidManagerValue other) {
  swap(*this, other);
  return *this;
}

std::string PmidManagerValue::Serialise() const {
  protobuf::PmidManagerValue value_proto;
  for (auto data_element : data_elements_) {
    auto data_element_proto(value_proto.add_data_elements());
    data_element_proto->set_data_type(
        static_cast<int32_t>(boost::apply_visitor(GetTagValueVisitor(), data_element.first)));
    data_element_proto->set_data_name(
        boost::apply_visitor(GetIdentityVisitor(), data_element.first).string());
    data_element_proto->set_size(data_element.second);
  }
  return value_proto.SerializeAsString();
}

void PmidManagerValue::Delete(const DataNameVariant& data_name) {
  auto identity(boost::apply_visitor(GetIdentityVisitor(), data_name));
  auto tag_value(boost::apply_visitor(GetTagValueVisitor(), data_name));
  auto iter(std::find_if(
                std::begin(data_elements_),
                std::end(data_elements_),
                [identity, tag_value](const ValueType& element)->bool {
                  return (boost::apply_visitor(GetIdentityVisitor(), element.first) == identity) &&
                         (boost::apply_visitor(GetTagValueVisitor(), element.first) == tag_value);
                   }));
  if (iter != std::end(data_elements_))
    data_elements_.erase(iter);
}

void PmidManagerValue::Add(const DataNameVariant& data_name, int32_t size) {
  data_elements_.insert(std::make_pair(data_name, size));
}

bool operator==(const PmidManagerValue& lhs, const PmidManagerValue& rhs) {
  if (lhs.data_elements_.size() != rhs.data_elements_.size())
    return false;
  auto mismatch(std::mismatch(std::begin(lhs.data_elements_),
                              std::end(lhs.data_elements_),
                              std::begin(rhs.data_elements_)));
  if (mismatch.first != std::end(lhs.data_elements_) ||
      mismatch.second != std::end(rhs.data_elements_))
    return false;
  return true;
}

void swap(PmidManagerValue& lhs, PmidManagerValue& rhs) {
  using std::swap;
  swap(lhs.data_elements_, rhs.data_elements_);
}

}  // namespace vault
}  // namespace maidsafe
