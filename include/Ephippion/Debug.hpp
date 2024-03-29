//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#ifdef BOOST_NO_EXCEPTIONS

#include <iostream>
// using std::cerr

#include <exception>
// using std::exception
// using std::terminate

namespace boost {

inline void throw_exception(std::exception const &e) {
  std::cerr << e.what() << '\n';

  std::terminate();
}

} // namespace boost

#endif // BOOST_NO_EXCEPTIONS

//

#if EPHIPPION_DEBUG

static bool dumpFunction(const llvm::Function *CurFunc = nullptr) {
  if (!CurFunc)
    return false;

  std::error_code ec;
  char filename[L_tmpnam];
  std::tmpnam(filename);

  llvm::raw_fd_ostream dbgFile(filename, ec, llvm::sys::fs::F_Text);

  llvm::errs() << "\nfunction dumped at: " << filename << "\n";
  CurFunc->print(dbgFile);

  return false;
}

#else

namespace llvm {
class Function;
} // namespace llvm

static constexpr bool dumpFunction(const llvm::Function *CurFunc = nullptr) {
  return true;
}

#endif // EPHIPPION_DEBUG

