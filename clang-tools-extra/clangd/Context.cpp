//===--- Context.cpp -----------------------------------------*- C++-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//

#include "Context.h"
#include <cassert>

namespace clang {
namespace clangd {

Context Context::empty() { return Context(/*Data=*/nullptr); }

Context::Context(std::shared_ptr<const Data> DataPtr)
    : DataPtr(std::move(DataPtr)) {}

Context Context::clone() const { return Context(DataPtr); }

// The thread-local Context is scoped in a function to avoid
// initialization-order issues. It's created when first needed.
static Context &currentContext() {
  static thread_local Context C = Context::empty();
  return C;
}

const Context &Context::current() { return currentContext(); }

Context Context::swapCurrent(Context Replacement) {
  std::swap(Replacement, currentContext());
  return Replacement;
}

} // namespace clangd
} // namespace clang
