// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Part of the R package 'highway'. The exception thrown by the patched default
// Highway abort handler (hwy::Abort, reached through HWY_ABORT and
// HWY_ASSERT). Upstream Highway prints to stderr and calls abort(); compiled
// code in R packages must never terminate the R process, so this package
// throws instead. Catch it at your .Call boundary (highway/r_guard.h, Rcpp or
// cpp11 do this) or at a C interface (highway/c_guard.h).

#ifndef HIGHWAY_ABORT_ERROR_H_
#define HIGHWAY_ABORT_ERROR_H_

#if !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && \
    !defined(_CPPUNWIND)
#error "The R package 'highway' requires C++ exceptions: Highway's abort " \
  "handler throws highway::abort_error instead of terminating R."
#endif

#include <stdexcept>

#include "highway/visibility.h"

namespace highway {

// Derived from std::runtime_error, so any catch (const std::exception&)
// handles it. what() is Highway's formatted message; file() and line() give
// the location of the failing HWY_ABORT/HWY_ASSERT. Copying never throws: the
// message is held by std::runtime_error and file() points into the string
// literal passed by HWY_ABORT (__FILE__).
class HIGHWAY_HIDDEN abort_error : public std::runtime_error {
 public:
  abort_error(const char* file, int line, const char* message)
      : std::runtime_error(message != nullptr ? message : ""),
        file_(file != nullptr ? file : ""),
        line_(line) {}

  const char* file() const noexcept { return file_; }
  int line() const noexcept { return line_; }

 private:
  const char* file_;
  int line_;
};

}  // namespace highway

#endif  // HIGHWAY_ABORT_ERROR_H_
