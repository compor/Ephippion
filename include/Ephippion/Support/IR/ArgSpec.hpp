//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/IR/ArgDirection.hpp"

namespace ephippion {

struct ArgSpec {
  ArgDirection Direction;
  bool IteratorDependent;
};

} // namespace ephippion

