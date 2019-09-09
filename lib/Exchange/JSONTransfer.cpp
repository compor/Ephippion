//
//
//

#include "Ephippion/Exchange/JSONTransfer.hpp"

// TODO maybe factor out this code to common utility project
//#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "llvm/Support/FileSystem.h"
// using llvm::sys::fs::F_Text

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Support/Path.h"
// using llvm::sys::path::filename

#include "llvm/Support/ToolOutputFile.h"
// using llvm::ToolOutputFile

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <fstream>

#include <streambuf>

#include <string>

#include <utility>
// using std::move

#include <system_error>
// using std::error_code

#if defined(__unix__) && defined(_POSIX_C_SOURCE)
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace ephippion {

void WriteJSONToFile(const llvm::json::Value &V,
                     const llvm::Twine &FilenamePrefix,
                     const llvm::Twine &Dir) {
  std::string absFilename{Dir.str() + "/" + FilenamePrefix.str() + ".json"};
  llvm::StringRef filename{llvm::sys::path::filename(absFilename)};
  llvm::errs() << "Writing file '" << filename << "'... ";

  std::error_code ec;
  llvm::ToolOutputFile of(absFilename, ec, llvm::sys::fs::F_Text);

  if (ec) {
    llvm::errs() << "error opening file '" << filename << "' for writing!\n";
    of.os().clear_error();
  }

  of.os() << llvm::formatv("{0:2}", V);
  of.os().close();

  if (!of.os().has_error()) {
    of.keep();
  }

  llvm::errs() << " done. \n";
}

llvm::ErrorOr<llvm::json::Value> ReadJSONFromFile(const llvm::Twine &Filename,
                                                  const llvm::Twine &Dir) {
  std::string absFilename{Dir.str()};
  if (absFilename != "") {
    absFilename += std::string{"/"};
  }
  absFilename += std::string{Filename.str()};

  llvm::StringRef filename{llvm::sys::path::filename(absFilename)};
  std::ifstream f;

  f.open(absFilename, std::ifstream::in);
  if (!f.is_open()) {
#if defined(__unix__) && defined(_POSIX_C_SOURCE)
    struct stat info;
    int rc = stat(filename.data(), &info);

    if (rc && errno == ENOENT) {
      return std::make_error_code(std::errc::no_such_file_or_directory);
    }
#endif

    return std::make_error_code(std::io_errc::stream);
  }

  std::string str{std::istreambuf_iterator<char>(f),
                  std::istreambuf_iterator<char>()};
  auto vOrError = llvm::json::parse(str);
  f.close();

  if (auto e = vOrError.takeError()) {
    return llvm::errorToErrorCode(std::move(e));
  }

  return *vOrError;
}

} // namespace ephippion

namespace llvm {
namespace json {

bool fromJSON(const Value &E, ephippion::ArgSpec &R) {
  ObjectMapper O(E);
  int direction;

  if (!O || !O.map("direction", direction) ||
      !O.map("iterator dependent", R.IteratorDependent)) {
    return false;
  }
  R.Direction = static_cast<ephippion::ArgDirection>(direction);

  return true;
}

} // namespace json
} // namespace llvm
