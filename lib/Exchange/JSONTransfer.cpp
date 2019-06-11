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

llvm::json::Value ReadJSONFromFile(const llvm::Twine &Filename) {
  std::ifstream f(Filename.str());

  std::string str((std::istreambuf_iterator<char>(f)),
                  std::istreambuf_iterator<char>());

  auto parsedOrError = llvm::json::parse(str);

  if (!parsedOrError) {
    llvm::report_fatal_error("JSON file parsing error!");
  }

  return *parsedOrError;
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
