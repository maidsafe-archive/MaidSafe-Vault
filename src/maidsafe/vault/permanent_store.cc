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

#include "maidsafe/common/data_stores/permanent_store.h"

#include <string>
#include <vector>

#include "boost/filesystem/convenience.hpp"
#include "boost/lexical_cast.hpp"

#include "maidsafe/common/log.h"
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

DiskUsage InitialiseDiskRoot(const fs::path& disk_root) {
  boost::system::error_code error_code;
  DiskUsage disk_usage(0);
  if (!fs::exists(disk_root, error_code)) {
    if (!fs::create_directories(disk_root, error_code)) {
      LOG(kError) << "Can't create disk root at " << disk_root << ": " << error_code.message();
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::uninitialised));
    }
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
  return disk_usage;
}

}  // unnamed namespace

PermanentStore::PermanentStore(const fs::path& disk_path, DiskUsage max_disk_usage)
    : kDiskPath_(disk_path),
      max_disk_usage_(std::move(max_disk_usage)),
      current_disk_usage_(InitialiseDiskRoot(kDiskPath_)),
      kDepth_(5),
      mutex_(),
      get_identity_visitor_() {
  if (current_disk_usage_ > max_disk_usage_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::cannot_exceed_limit));
}

PermanentStore::~PermanentStore() {}

void PermanentStore::Put(const KeyType& key, const NonEmptyString& value) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!fs::exists(kDiskPath_)) {
    LOG(kError) << "PermanentStore::Put kDiskPath_ " << kDiskPath_ << " doesn't exists";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }

  fs::path file_path(KeyToFilePath(key));
  LOG(kVerbose) << "PermanentStore::Put file_path " << file_path;
  uint32_t value_size(static_cast<uint32_t>(value.string().size()));
  uint64_t file_size(0), size(0);
  bool increment(true);
  boost::system::error_code error_code;

  if (fs::exists(file_path, error_code)) {
    if (error_code) {
      LOG(kError) << "Unable to determine file status for " << file_path << ": "
                  << error_code.message();
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
    }
    file_size = fs::file_size(file_path, error_code);
    if (error_code) {
      LOG(kError) << "Error getting file size of " << file_path << ": " << error_code.message();
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
    }
  }
  if (file_size != 0) {
    if (file_size <= value_size) {
      size = value_size - file_size;
    } else {
      size = file_size - value_size;
      increment = false;
    }
  } else {
    size = value_size;
  }

  if (increment) {
    if (!HasDiskSpace(size)) {
      LOG(kError) << "Cannot store "
                  << HexSubstr(boost::apply_visitor(get_identity_visitor_, key).string())
                  << " since the addition of " << size << " bytes exceeds max of "
                  << max_disk_usage_ << " bytes.";
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::cannot_exceed_limit));
    }
  }
  if (!WriteFile(file_path, value.string())) {
    LOG(kError) << "Failed to write "
                << HexSubstr(boost::apply_visitor(get_identity_visitor_, key).string())
                << " to disk.";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }

  if (increment) {
    current_disk_usage_.data += size;
  } else {
    current_disk_usage_.data -= size;
  }
}

void PermanentStore::Delete(const KeyType& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  fs::path path(KeyToFilePath(key));
  boost::system::error_code error_code;
  uint64_t file_size(fs::file_size(path, error_code));
  if (error_code) {
    LOG(kError) << "Error getting file size of " << path << ": " << error_code.message();
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }
  if (!fs::remove(path, error_code) || error_code) {
    LOG(kError) << "Error removing " << path << ": " << error_code.message();
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  }
  current_disk_usage_.data -= file_size;
}

NonEmptyString PermanentStore::Get(const KeyType& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return ReadFile(KeyToFilePath(key));
}

void PermanentStore::SetMaxDiskUsage(DiskUsage max_disk_usage) {
  if (current_disk_usage_ > max_disk_usage) {
    LOG(kError) << "current_disk_usage_ " << current_disk_usage_.data
                << " exceeds target max_disk_usage " << max_disk_usage.data;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  max_disk_usage_ = max_disk_usage;
}

std::vector<PermanentStore::KeyType> PermanentStore::GetKeys() const {
  std::vector<DataNameVariant> keys;
  fs::directory_iterator end_iter;

  if (fs::exists(kDiskPath_) && fs::is_directory(kDiskPath_)) {
    for (fs::directory_iterator dir_iter(kDiskPath_); dir_iter != end_iter; ++dir_iter) {
      if (fs::is_regular_file(dir_iter->status()))
        keys.push_back(detail::GetDataNameVariant(*dir_iter));
    }
  }
  return keys;
}

fs::path PermanentStore::GetFilePath(const KeyType& key) const {
  return kDiskPath_ / detail::GetFileName(key);
}

bool PermanentStore::HasDiskSpace(uint64_t required_space) const {
  return current_disk_usage_ + required_space <= max_disk_usage_;
}

fs::path PermanentStore::KeyToFilePath(const KeyType& key) const {
  NonEmptyString file_name(GetFilePath(key).filename().string());

  uint32_t directory_depth = kDepth_;
  if (file_name.string().length() < directory_depth)
    directory_depth = static_cast<uint32_t>(file_name.string().length() - 1);

  fs::path disk_path(kDiskPath_);
  for (uint32_t i = 0; i < directory_depth; ++i)
    disk_path /= file_name.string().substr(i, 1);

  boost::system::error_code ec;
  fs::create_directories(disk_path, ec);

  return fs::path(disk_path / file_name.string().substr(directory_depth));
}

}  // namespace data_stores

}  // namespace maidsafe
