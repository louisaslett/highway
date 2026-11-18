// Copyright 2019 Google LLC
// Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: BSD-3-Clause
//
// Modified for the R package 'highway' (local patch 0001): compiled code in
// an R package must not write to stderr or terminate the R process. The
// default Warn() is therefore silent and the default Abort() throws
// highway::abort_error instead of printing and calling abort()/exit().

#include "hwy/abort.h"

#include <stdarg.h>
#include <stdio.h>  // vsnprintf

#include <atomic>

#include "highway/abort_error.h"
#include "hwy/base.h"

namespace hwy {

namespace {

std::atomic<WarnFunc>& AtomicWarnFunc() {
  static std::atomic<WarnFunc> func;
  return func;
}

std::atomic<AbortFunc>& AtomicAbortFunc() {
  static std::atomic<AbortFunc> func;
  return func;
}

// Returns a pointer into `file_name` (normally the __FILE__ literal), so the
// result has static storage duration and can be kept by highway::abort_error.
const char* GetBaseName(const char* file_name) {
  if (file_name == nullptr) return "";
  const char* base = file_name;
  for (const char* p = file_name; *p != '\0'; ++p) {
    if (*p == '/' || *p == '\\') base = p + 1;
  }
  return base;
}

}  // namespace

// Returning a reference is unfortunately incompatible with `std::atomic`, which
// is required to safely implement `SetWarnFunc`. As a workaround, we store a
// copy here, update it when called, and return a reference to the copy. This
// has the added benefit of protecting the actual pointer from modification.
HWY_DLLEXPORT WarnFunc& GetWarnFunc() {
  static WarnFunc func;
  func = AtomicWarnFunc().load();
  return func;
}

HWY_DLLEXPORT AbortFunc& GetAbortFunc() {
  static AbortFunc func;
  func = AtomicAbortFunc().load();
  return func;
}

HWY_DLLEXPORT WarnFunc SetWarnFunc(WarnFunc func) {
  return AtomicWarnFunc().exchange(func);
}

HWY_DLLEXPORT AbortFunc SetAbortFunc(AbortFunc func) {
  return AtomicAbortFunc().exchange(func);
}

HWY_DLLEXPORT void HWY_FORMAT(3, 4)
    Warn(const char* file, int line, const char* format, ...) {
  char buf[800];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  // R package 'highway': no default output. Warn() may run on threads that R
  // does not own, so the default must not print or call the R API.
  WarnFunc handler = AtomicWarnFunc().load();
  if (handler != nullptr) {
    handler(file, line, buf);
  }
}

HWY_DLLEXPORT HWY_NORETURN void HWY_FORMAT(3, 4)
    Abort(const char* file, int line, const char* format, ...) {
  char buf[800];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  AbortFunc handler = AtomicAbortFunc().load();
  if (handler != nullptr) {
    handler(file, line, buf);
  }

  // R package 'highway': never terminate the R process. If no handler is set,
  // or the handler returns, unwind to the caller's .Call or C guard.
  throw ::highway::abort_error(GetBaseName(file), line, buf);
}

}  // namespace hwy
