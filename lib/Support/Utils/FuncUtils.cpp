//
//
//

#include "Ephippion/Support/Utils/FuncUtils.hpp"

#include "llvm/IR/LLVMContext.h"
// using llvm::LLVMContext

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/ADT/ArrayRef.h"
// using llvm::ArrayRef

llvm::Function *ephippion::DeclareFunc(llvm::Module &M, llvm::StringRef Name,
                                       llvm::Type *RetTy,
                                       llvm::ArrayRef<llvm::Type *> ArgsTy) {
  return llvm::cast<llvm::Function>(M.getOrInsertFunction(
      Name, llvm::FunctionType::get(RetTy, ArgsTy, false)));
}

llvm::Function *ephippion::DeclareFunc(llvm::Module &M, llvm::StringRef Name,
                                       llvm::Type *RetTy) {
  return llvm::cast<llvm::Function>(
      M.getOrInsertFunction(Name, llvm::FunctionType::get(RetTy, false)));
}

llvm::Function *ephippion::DeclareMallocLikeFunc(llvm::Module &M,
                                                 llvm::StringRef Name) {
  return DeclareFunc(M, Name, llvm::Type::getInt8PtrTy(M.getContext()),
                     {llvm::Type::getInt64Ty(M.getContext())});
}

llvm::Function *ephippion::DeclareFreeLikeFunc(llvm::Module &M,
                                               llvm::StringRef Name) {
  return DeclareFunc(M, Name, llvm::Type::getVoidTy(M.getContext()),
                     {llvm::Type::getInt8PtrTy(M.getContext())});
}

llvm::Function *ephippion::DeclareMemcmpLikeFunc(llvm::Module &M,
                                                 llvm::StringRef Name) {
  auto &curCtx = M.getContext();
  auto *ptrType = llvm::Type::getInt8PtrTy(curCtx);
  return DeclareFunc(M, Name, llvm::Type::getInt32Ty(curCtx),
                     {ptrType, ptrType, llvm::Type::getInt64Ty(curCtx)});
}

llvm::Function *ephippion::DeclareMemcpyLikeFunc(llvm::Module &M,
                                                 llvm::StringRef Name) {
  auto &curCtx = M.getContext();
  auto *ptrType = llvm::Type::getInt8PtrTy(curCtx);
  return DeclareFunc(M, Name, ptrType,
                     {ptrType, ptrType, llvm::Type::getInt64Ty(curCtx)});
}

llvm::Function *ephippion::DeclareKLEELikeFunc(llvm::Module &M,
                                               llvm::StringRef Name) {
  return llvm::cast<llvm::Function>(M.getOrInsertFunction(
      Name, llvm::FunctionType::get(llvm::Type::getInt32Ty(M.getContext()), {},
                                    true)));
}

llvm::Function *ephippion::DeclareAssertLikeFunc(llvm::Module &M,
                                                 llvm::StringRef Name) {
  auto &curCtx = M.getContext();
  auto *ptrType = llvm::Type::getInt8PtrTy(curCtx);
  return llvm::cast<llvm::Function>(M.getOrInsertFunction(
      Name,
      llvm::FunctionType::get(
          llvm::Type::getVoidTy(curCtx),
          {ptrType, ptrType, llvm::Type::getInt32Ty(M.getContext()), ptrType},
          false)));
}

