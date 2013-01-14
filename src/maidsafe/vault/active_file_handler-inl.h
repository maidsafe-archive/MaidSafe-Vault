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

#ifndef MAIDSAFE_VAULT_ACTIVE_FILE_HANDLER_INL_H_
#define MAIDSAFE_VAULT_ACTIVE_FILE_HANDLER_INL_H_

#include "boost/filesystem/operations.hpp"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace gp = google::protobuf;

namespace maidsafe {

namespace vault {

template <typename C, typename E>
ActiveFileHandler<C, E>::ActiveFileHandler(const boost::filesystem::path& base_path,
                                        const boost::filesystem::path& file_name)
    : active_(),
      kMaxElementCount_(1000),
      current_element_count_(0),
      current_file_(0),
      file_name_(file_name),
      base_path_(base_path),
      curent_file_path_(base_path_ /
                        (file_name_.string() + boost::lexical_cast<std::string>(current_file_))) {
  // Check for files on the given path
  CheckForExistingFiles();
}

template <typename C, typename E>
void ActiveFileHandler<C, E>::CheckForExistingFiles() {
  if (boost::filesystem::is_empty(base_path_)) {
    LOG(kInfo) << "Empty directory. Fine to use.";
    return;
  }

  // Check all elements to see if they have the name of the class
  boost::filesystem::directory_iterator end_iter;
  int largest_file_index(-1), current_file_index(-1);
  for (boost::filesystem::directory_iterator dir_iter(base_path_);
       dir_iter != end_iter;
       ++dir_iter) {
    std::string full_file_name((boost::filesystem::path(*dir_iter)).filename().string());
    if (full_file_name.find(file_name_.string()) == std::string::npos) {
      LOG(kError) << "File of different name in directory.";
      throw std::exception();
    }
    current_file_index =
        boost::lexical_cast<int>(full_file_name.substr(file_name_.string().size()));
    if (current_file_index > largest_file_index)
      largest_file_index = current_file_index;
  }

  if (largest_file_index > current_file_) {
    current_file_ = largest_file_index;
    curent_file_path_ = base_path_ /
                        (file_name_.string() + boost::lexical_cast<std::string>(current_file_));
  }

  // Read the largest file
  NonEmptyString content(ReadFile(curent_file_path_));
  C container;
  if (!container.ParseFromString(content.string())) {
    LOG(kError) << "Content in file doesn't parse: " << curent_file_path_;
    throw std::exception();
  }

  std::string field_name(container.GetDescriptor()->field(0)->full_name());
  current_element_count_ =
      static_cast<int>(container.GetReflection()->FieldSize(&container, field_name));
}

template <typename C, typename E>
void ActiveFileHandler<C, E>::Write(Identity name, const E& container_element) {
  active_.Send([=] () { this->DoWrite(name, container_element); });  // NOLINT (Dan)
}

template <typename C, typename E>
void ActiveFileHandler<C, E>::Delete(Identity name, const E& container_element) {
  active_.Send([=] () { this->DoDelete(name, container_element); });  // NOLINT (Dan)
}

template <typename C, typename E>
void ActiveFileHandler<C, E>::DoWrite(Identity name, const E& container_element) {

}

template <typename C, typename E>
void ActiveFileHandler<C, E>::DoDelete(Identity name, E container_element) {

}

template <typename C, typename E>
void ActiveFileHandler<C, E>::SearchFilesForIndex() {
  C container;
  NonEmptyString content;
  for (int n(current_file_); n != -1; --n) {
    std::string file_name(file_name_.string() + boost::lexical_cast<std::string>(n));
    content = ReadFile(base_path_ / file_name);
    container.Clear();
    if (!container.ParseFromString()) {
      LOG(kInfo) << "Failed to parse. Skipping file " << file_name;
      continue;
    }

    // Check index to find the correct one
    const gp::FieldDescriptor* field_descriptor_0(container.GetDescriptor()->field(0));
    std::string field_name(field_descriptor_0->full_name());
    const gp::Reflection* reflection(container.GetReflection());
    int count(static_cast<int>(reflection->FieldSize(&container, field_name)));
    for (int n(0); n != count; ++count) {
      E element(reflection->GetRepeatedMessage(container, field_descriptor_0, n));
      // Check element
    }
  }
}

template <typename C, typename E>
void ActiveFileHandler<C, E>::IncreaseFileContainer() {
  if (++current_element_count_ == kMaxElementCount_) {
    ++current_file_;
    curent_file_path_ = base_path_ /
                        (file_name_.string() + boost::lexical_cast<std::string>(current_file_));
  }
}

template <typename C, typename E>
void ActiveFileHandler<C, E>::DecreaseFileContainer(int file_index, int file_element_count) {
  if (file_element_count == 0) {
    boost::filesystem::remove(base_path_ /
                              (file_name_.string() + boost::lexical_cast<std::string>(file_index)));
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACTIVE_FILE_HANDLER_INL_H_
