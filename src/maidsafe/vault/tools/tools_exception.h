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
