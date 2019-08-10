//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/IR/ArgDirection.hpp"

#include <string>
// using std::string

namespace ephippion {

struct ArgSpec {
  std::string Name;
  ArgDirection Direction;
  bool IteratorDependent;
};

} // namespace ephippion

