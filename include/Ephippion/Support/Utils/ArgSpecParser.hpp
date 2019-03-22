//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "Ephippion/Support/IR/ArgSpec.hpp"

#include "llvm/Support/CommandLine.h"
// using llvm::cl::basic_parser
// using llvm::cl::parser
// using llvm::cl::Option

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include "llvm/ADT/StringSwitch.h"
// using llvm::StringSwitch

#include "llvm/Support/Debug.h"
// using llvm::dbgs
// using LLVM_DEBUG

#include <regex>
// using std::regex

#include <string>
// using std::string

#include <cstdlib>
// using std::strtoul

namespace llvm {
namespace cl {

template <>
class parser<ephippion::ArgSpec> : public basic_parser<ephippion::ArgSpec> {
public:
  parser(Option &O) : basic_parser(O) {}

  bool parse(Option &O, StringRef ArgName, const std::string &ArgValue,
             ephippion::ArgSpec &Val) {
    std::regex optre{"([[:alpha:]]+):([0|1])"};
    std::smatch matches;

    bool errorOccurred = true;

    if (std::regex_match(ArgValue, matches, optre)) {
      if (matches.size() == 3) {
        Val.Direction = StringSwitch<ephippion::ArgDirection>(matches[1].str())
                            .Case("in", ephippion::ArgDirection::AD_Inbound)
                            .Case("out", ephippion::ArgDirection::AD_Outbound)
                            .Case("both", ephippion::ArgDirection::AD_Both)
                            .Default(ephippion::ArgDirection::AD_Inbound);

        char *c;
        Val.IteratorDependent = std::strtoul(matches[2].str().c_str(), &c, 10);

        errorOccurred = false;
      }
    }

    if (errorOccurred) {
      return O.error("'" + ArgValue + "' value invalid for argspec argument!");
    }

    return errorOccurred;
  }
};

} // namespace cl
} // namespace llvm
