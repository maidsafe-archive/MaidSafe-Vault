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

#ifndef MAIDSAFE_VAULT_TOOLS_TOOLS_EXCEPTION_H_
#define MAIDSAFE_VAULT_TOOLS_TOOLS_EXCEPTION_H_

#include <exception>

namespace maidsafe {

namespace vault {

namespace tools {

class ToolsException: public std::exception {
 public:
  ToolsException(const std::string& message) : message_(message) {}
  virtual const char* what() const throw() { return message_.c_str(); }

 private:
  std::string message_;
};

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_TOOLS_TOOLS_EXCEPTION_H_
