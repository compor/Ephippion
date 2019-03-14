//
//
//

#pragma once

#include "Ephippion/Config.hpp"

#include "llvm/ADT/ArrayRef.h"
// using llvm::ArrayRef

namespace llvm {
class Module;
class Function;
class Type;
class StringRef;
} // namespace llvm

namespace ephippion {

llvm::Function *DeclareFunc(llvm::Module &M, llvm::StringRef Name,
                            llvm::Type *RetTy,
                            llvm::ArrayRef<llvm::Type *> ArgsTy);

llvm::Function *DeclareMallocLikeFunc(llvm::Module &M, llvm::StringRef Name);

llvm::Function *DeclareFreeLikeFunc(llvm::Module &M, llvm::StringRef Name);

} // namespace ephippion
