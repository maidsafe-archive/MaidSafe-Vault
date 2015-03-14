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

#include "maidsafe/vault/version_handler/version_handler.h"

#include "maidsafe/common/convert.h"

namespace maidsafe {

namespace vault {

template <typename FacadeType>
bool VersionHandler<FacadeType>::HandlePost(const routing::SerialisedMessage& message) {
  InputVectorStream binary_input_stream { message };
  Identity sdv_name;
  StructuredDataVersions::VersionName new_version, old_version;
  Parse(binary_input_stream, sdv_name, old_version, new_version);
  std::string key(convert::ToString(sdv_name.string()));
  try {
    std::string serialised_sdv;
    db_.Get(key, serialised_sdv);
    MutableData sdv_wrapper(Parse<MutableData>(convert::ToByteVector(serialised_sdv)));
    StructuredDataVersions sdv(20, 1);
    sdv.ApplySerialised(StructuredDataVersions::serialised_type(sdv_wrapper.Value()));
    sdv.Put(old_version, new_version);
  } catch (...) {
    return false;
  }
  return true;
}

}  // namespace vault

}  // namespace maidsafe
