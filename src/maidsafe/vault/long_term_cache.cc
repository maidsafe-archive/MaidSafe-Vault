/*  Copyright 2013 MaidSafe.net limited

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

#include "maidsafe/common/data_stores/local_store.h"

#include <string>
#include <vector>

#include "boost/filesystem/convenience.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/make_unique.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/data_stores/utils.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace data_stores {

namespace {

struct UsedSpace {
  UsedSpace() {}
  UsedSpace(UsedSpace&& other)
      : directories(std::move(other.directories)), disk_usage(std::move(other.disk_usage)) {}

  std::vector<fs::path> directories;
  DiskUsage disk_usage;
};

/*
UsedSpace GetUsedSpace(fs::path directory) {
  UsedSpace used_space;
  for (fs::directory_iterator it(directory); it != fs::directory_iterator(); ++it) {
    if (fs::is_directory(*it))
      used_space.directories.push_back(it->path());
    else
      used_space.disk_usage.data += fs::file_size(*it);
  }
  return used_space;
}
*/

DiskUsage InitialiseDiskRoot(const fs::path& disk_root) {
  boost::system::error_code error_code;
  DiskUsage disk_usage(0);
  if (!fs::exists(disk_root, error_code)) {
    if (!fs::create_directories(disk_root, error_code)) {
      LOG(kError) << "Can't create disk root at " << disk_root << ": " << error_code.message();
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::uninitialised));
    }
  }
  // TODO(Fraser#5#): 2014-01-30 - BEFORE_RELEASE re-enable this functionality using a different
  //                               (much faster) method.
/*
  } else {
    std::vector<fs::path> dirs_to_do;
    dirs_to_do.push_back(disk_root);
    while (!dirs_to_do.empty()) {
      std::vector<std::future<UsedSpace>> futures;
      for (uint32_t i = 0; i < 16 && !dirs_to_do.empty(); ++i) {
        auto future = std::async(&GetUsedSpace, dirs_to_do.back());
        dirs_to_do.pop_back();
        futures.push_back(std::move(future));
      }
      try {
        while (!futures.empty()) {
          auto future = std::move(futures.back());
          futures.pop_back();
          UsedSpace result = future.get();
          disk_usage.data += result.disk_usage.data;
          std::copy(result.directories.begin(), result.directories.end(),
                    std::back_inserter(dirs_to_do));
        }
      }
      catch (const std::system_error& exception) {
        LOG(kError) << boost::diagnostic_information(exception);
        BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
      }
      catch (...) {
        LOG(kError) << "exception during InitialiseDiskRoot";
        BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
      }
    }
  }
*/
  return disk_usage;
}

}  // unnamed namespace

LocalStore::LocalStore(const fs::path& disk_path, DiskUsage max_disk_usage)
    : asio_service_(Concurrency() / 2),  // TODO(Fraser#5#): 2013-09-06 - determine best value.
      kDiskPath_(disk_path),
      max_disk_usage_(std::move(max_disk_usage)),
      current_disk_usage_(InitialiseDiskRoot(kDiskPath_)),
      kDepth_(5),
      get_identity_visitor_() {
  if (current_disk_usage_ > max_disk_usage_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::cannot_exceed_limit));
}

LocalStore::~LocalStore() { asio_service_.Stop(); }

NonEmptyString LocalStore::DoGet(const KeyType& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  fs::path file_path(KeyToFilePath(key, false));
  uint32_t reference_count(GetReferenceCount(file_path));
  file_path.replace_extension("." + std::to_string(reference_count));
  return ReadFile(file_path);
}

void LocalStore::DoPut(const KeyType& key, const NonEmptyString& value) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!fs::exists(kDiskPath_))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));

  fs::path file_path(KeyToFilePath(key, true));
  uint32_t value_size(static_cast<uint32_t>(value.string().size()));
  uintmax_t file_size(0);
  uint32_t reference_count(GetReferenceCount(file_path));
  DataTagValue data_tag_value(boost::apply_visitor(GetTagValueVisitor(), key));

  if (reference_count == 0) {
    file_path.replace_extension(".1");
    Write(file_path, value, value_size);
    current_disk_usage_.data += value_size;
  } else if (data_tag_value == DataTagValue::kImmutableDataValue) {
    fs::path old_path(file_path), new_path(file_path);
    old_path.replace_extension("." + std::to_string(reference_count));
    ++reference_count;
    new_path.replace_extension("." + std::to_string(reference_count));
    file_size = Rename(old_path, new_path);
    assert(file_size == value_size);
  } else {
    assert(reference_count == 1);
    file_path.replace_extension(".1");
    file_size = Remove(file_path);
    current_disk_usage_.data -= file_size;
    Write(file_path, value, value_size);
    current_disk_usage_.data += value_size;
  }
}

void LocalStore::DoDelete(const KeyType& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  fs::path file_path(KeyToFilePath(key, false));
  uintmax_t file_size(0);
  uint32_t reference_count(GetReferenceCount(file_path));

  if (reference_count == 0) {
    LOG(kWarning) << HexSubstr(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key).second)
                  << " already deleted.";
    return;
  }

  if (reference_count == 1) {
    file_path.replace_extension(".1");
    file_size = Remove(file_path);
    current_disk_usage_.data -= file_size;
  } else {
    fs::path new_path(file_path);
    file_path.replace_extension("." + std::to_string(reference_count));
    --reference_count;
    new_path.replace_extension("." + std::to_string(reference_count));
    file_size = Rename(file_path, new_path);
    assert(file_size != 0);
  }
}

void LocalStore::IncrementReferenceCount(const std::vector<ImmutableData::Name>& data_names) {
  asio_service_.service().post([this, data_names] {
    try {
      DoIncrement(data_names);
    }
    catch (const std::exception& e) {
      LOG(kWarning) << "IncrementReferenceCount failed: " << boost::diagnostic_information(e);
    }
  });
}

void LocalStore::DecrementReferenceCount(const std::vector<ImmutableData::Name>& data_names) {
  asio_service_.service().post([this, data_names] {
    try {
      DoDecrement(data_names);
    }
    catch (const std::exception& e) {
      LOG(kWarning) << "DecrementReferenceCount failed: " << boost::diagnostic_information(e);
    }
  });
}

void LocalStore::DoIncrement(const std::vector<ImmutableData::Name>& data_names) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!fs::exists(kDiskPath_))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));

  for (const auto& data_name : data_names) {
    fs::path file_path(KeyToFilePath(data_name, false));
    uint32_t reference_count(GetReferenceCount(file_path));
    assert(reference_count != 0);

    fs::path old_path(file_path), new_path(file_path);
    old_path.replace_extension("." + std::to_string(reference_count));
    ++reference_count;
    new_path.replace_extension("." + std::to_string(reference_count));
    auto file_size(Rename(old_path, new_path));
    assert(file_size != 0);
    static_cast<void>(file_size);
  }
}

void LocalStore::DoDecrement(const std::vector<ImmutableData::Name>& data_names) {
  for (const auto& data_name : data_names)
    DoDelete(data_name);
}

void LocalStore::SetMaxDiskUsage(DiskUsage max_disk_usage) {
  if (current_disk_usage_ > max_disk_usage) {
    LOG(kError) << "current_disk_usage_ " << current_disk_usage_.data
                << " exceeds target max_disk_usage " << max_disk_usage.data;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  max_disk_usage_ = max_disk_usage;
}

DiskUsage LocalStore::GetMaxDiskUsage() const { return max_disk_usage_; }

DiskUsage LocalStore::GetCurrentDiskUsage() const { return current_disk_usage_; }

fs::path LocalStore::GetFilePath(const KeyType& key) const {
  return kDiskPath_ / detail::GetFileName(key);
}

bool LocalStore::HasDiskSpace(uint64_t required_space) const {
  return current_disk_usage_ + required_space <= max_disk_usage_;
}

fs::path LocalStore::KeyToFilePath(const KeyType& key, bool create_if_missing) const {
  NonEmptyString file_name(GetFilePath(key).filename().string());

  uint32_t directory_depth = kDepth_;
  if (file_name.string().length() < directory_depth)
    directory_depth = static_cast<uint32_t>(file_name.string().length() - 1);

  fs::path disk_path(kDiskPath_);
  for (uint32_t i = 0; i < directory_depth; ++i)
    disk_path /= file_name.string().substr(i, 1);

  if (create_if_missing) {
    boost::system::error_code ec;
    fs::create_directories(disk_path, ec);
  }

  return fs::path(disk_path / file_name.string().substr(directory_depth));
}

uint32_t LocalStore::GetReferenceCount(const fs::path& path) const {
  boost::system::error_code error_code;
  if (!fs::exists(path.parent_path(), error_code)) {
    LOG(kWarning) << path << " doesn't exist.";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
  }

  try {
    std::string file_name(path.filename().string());
    fs::directory_iterator end;
    for (fs::directory_iterator it(path.parent_path()); it != end; ++it) {
      if (it->path().stem().string() == file_name && fs::is_regular_file(it->status()))
        return std::stoul(it->path().extension().string().substr(1));
    }
  }
  catch (const std::exception& e) {
    LOG(kError) << "Exception: " << boost::diagnostic_information(e);
  }

  return 0;
}

void LocalStore::Write(const boost::filesystem::path& path, const NonEmptyString& value,
                          const uintmax_t& size) {
  if (!HasDiskSpace(size)) {
    LOG(kError) << "Out of space.";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::cannot_exceed_limit));
  }
  if (!WriteFile(path, value.string())) {
    LOG(kError) << "Write failed.";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }
}

uintmax_t LocalStore::Remove(const fs::path& path) {
  boost::system::error_code error_code;
  uintmax_t file_size = fs::file_size(path, error_code);
  if (error_code) {
    LOG(kError) << "Error getting file size of " << path << ": " << error_code.message();
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }
  if (!fs::remove(path, error_code) || error_code) {
    LOG(kError) << "Error removing file " << path << ": " << error_code.message();
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }
  return file_size;
}

uintmax_t LocalStore::Rename(const fs::path& old_path, const fs::path& new_path) {
  boost::system::error_code error_code;
  uintmax_t file_size = fs::file_size(old_path, error_code);
  if (error_code) {
    LOG(kError) << "Error getting file size of " << old_path << ": " << error_code.message();
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }
  fs::rename(old_path, new_path, error_code);
  if (error_code) {
    LOG(kError) << "Error renaming file " << old_path << ": " << error_code.message();
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }
  return file_size;
}

std::unique_ptr<StructuredDataVersions> LocalStore::ReadVersions(const KeyType& key) const {
  fs::path file_path(KeyToFilePath(key, false));
  file_path.replace_extension(".ver");
  boost::system::error_code ec;
  return fs::exists(file_path, ec) ?
      std::move(maidsafe::make_unique<StructuredDataVersions>(
          StructuredDataVersions::serialised_type(ReadFile(file_path)))) :
      std::move(std::unique_ptr<StructuredDataVersions>());
}

void LocalStore::WriteVersions(const KeyType& key, const StructuredDataVersions& versions) {
  if (!fs::exists(kDiskPath_))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));

  fs::path file_path(KeyToFilePath(key, true));
  file_path.replace_extension(".ver");

  boost::system::error_code ec;
  if (fs::exists(file_path, ec))
    current_disk_usage_.data -= fs::file_size(file_path, ec);

  auto serialised_versions(versions.Serialise().data);
  uint32_t value_size(static_cast<uint32_t>(serialised_versions.string().size()));
  Write(file_path, serialised_versions, value_size);
  current_disk_usage_.data += value_size;
}

}  // namespace data_stores

}  // namespace maidsafe
