//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/IR/ArgSpec.hpp"

#include "Ephippion/Exchange/Info.hpp"

#include "llvm/ADT/ArrayRef.h"
// usign llvm::ArrayRef

#include "llvm/Support/JSON.h"
// using json::Value
// using json::Object
// using json::Array

namespace ephippion {

void WriteJSONToFile(const llvm::json::Value &V,
                     const llvm::Twine &FilenamePrefix, const llvm::Twine &Dir);

llvm::json::Value ReadJSONFromFile(const llvm::Twine &Filename);

} // namespace ephippion

namespace llvm {
namespace json {

bool fromJSON(const Value &E, ephippion::ArgSpec &R);

} // namespace json
} // namespace llvm

