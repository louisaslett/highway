// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Part of the R package 'highway'. Guards for C interfaces.
//
// A C++ exception must never propagate into C code, into a function called
// through a C function pointer, or out of a thread's entry function: any of
// these terminates the process (std::terminate), taking R down with it. Wrap
// every such entry point in highway::c_guard(), which runs a callable, catches
// everything (including highway::abort_error from Highway's abort handler) and
// returns a status code from highway/c_status.h, copying the error message
// into a caller-supplied buffer.
//
//     extern "C" int mypkg_exp(const double* x, double* out, size_t n,
//                              char* err, size_t err_len) {
//       return highway::c_guard([&] { CallExpKernel(x, out, n); }, err, err_len);
//     }
//
// The same pattern is available as a macro pair for code that prefers it:
//
//     int status;
//     HIGHWAY_C_BEGIN(err, err_len)
//       CallExpKernel(x, out, n);
//     HIGHWAY_C_END(status)
//
// Nothing here calls the R API; c_guard() is safe on any thread.

#ifndef HIGHWAY_C_GUARD_H_
#define HIGHWAY_C_GUARD_H_

#include <stddef.h>
#include <string.h>

#include <exception>
#include <initializer_list>
#include <new>
#include <utility>

#include "highway/abort_error.h"
#include "highway/c_status.h"
#include "highway/visibility.h"

namespace highway {
namespace detail {

// Copies `msg` (truncated, always NUL-terminated) into buf[0, len).
HIGHWAY_HIDDEN inline void copy_message(char* buf, size_t len,
                                        const char* prefix,
                                        const char* msg) noexcept {
  if (buf == nullptr || len == 0) return;
  size_t pos = 0;
  for (const char* s : {prefix, msg}) {
    if (s == nullptr) continue;
    const size_t n = strlen(s);
    const size_t room = len - 1 - pos;
    const size_t take = n < room ? n : room;
    memcpy(buf + pos, s, take);
    pos += take;
  }
  buf[pos] = '\0';
}

// Classifies the exception currently being handled. Call only from inside a
// catch block.
HIGHWAY_HIDDEN inline int status_from_current_exception(char* buf,
                                                        size_t len) noexcept {
  try {
    throw;
  } catch (const ::highway::abort_error& e) {
    copy_message(buf, len, "Highway abort: ", e.what());
    return HIGHWAY_STATUS_ABORT;
  } catch (const std::bad_alloc&) {
    copy_message(buf, len, "", "memory allocation failed");
    return HIGHWAY_STATUS_NO_MEMORY;
  } catch (const std::exception& e) {
    copy_message(buf, len, "", e.what());
    return HIGHWAY_STATUS_EXCEPTION;
  } catch (...) {
    copy_message(buf, len, "", "unknown C++ exception");
    return HIGHWAY_STATUS_UNKNOWN;
  }
}

}  // namespace detail

// Runs `f()` and returns HIGHWAY_STATUS_OK, or the status for the exception it
// threw. `errbuf` (may be null) receives a NUL-terminated message, or "" on
// success. Never throws. `f` must not call the R API if this runs on a thread
// other than R's main thread.
template <class F>
HIGHWAY_HIDDEN inline int c_guard(F&& f, char* errbuf = nullptr,
                                  size_t errlen = 0) noexcept {
  if (errbuf != nullptr && errlen > 0) errbuf[0] = '\0';
  try {
    std::forward<F>(f)();
    return HIGHWAY_STATUS_OK;
  } catch (...) {
    return detail::status_from_current_exception(errbuf, errlen);
  }
}

}  // namespace highway

// Macro form. `buf`/`len` may be a null pointer and 0. After HIGHWAY_C_END,
// `status_var` (an int lvalue) holds the status. Objects declared between the
// two macros are destroyed before the status is assigned.
#define HIGHWAY_C_BEGIN(buf, len)                                  \
  {                                                                \
    char* highway_c_buf_ = (buf);                                  \
    const size_t highway_c_len_ = (len);                           \
    int highway_c_status_ = HIGHWAY_STATUS_OK;                     \
    if (highway_c_buf_ != nullptr && highway_c_len_ > 0)           \
      highway_c_buf_[0] = '\0';                                    \
    try {
#define HIGHWAY_C_END(status_var)                                  \
    } catch (...) {                                                \
      highway_c_status_ = ::highway::detail::                      \
          status_from_current_exception(highway_c_buf_,            \
                                        highway_c_len_);           \
    }                                                              \
    (status_var) = highway_c_status_;                              \
  }

#endif  // HIGHWAY_C_GUARD_H_
