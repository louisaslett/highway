// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Part of the R package 'highway'. Single-file implementation of Highway's
// compiled runtime, in the style of stb single-header libraries.
//
// Highway's dynamic dispatch (HWY_DYNAMIC_DISPATCH) needs a few non-inline
// functions: CPU detection and the chosen-target state, the abort/warning
// handlers, the aligned allocator, and a few per-target queries. Upstream
// builds them as a library; R packages cannot link against another package's
// shared library, so each package that uses dynamic dispatch compiles them
// itself by including this header in EXACTLY ONE C++ source file:
//
//     // src/highway-impl.cpp
//     #include "highway/impl.h"
//
// use_highway() creates that file. Keep it on its own: do not put kernels or
// other includes in it.
//
//   * Included in no file: loading the package fails with an undefined symbol
//     such as hwy::SupportedTargets() or hwy::Abort(...).
//   * Included in two files: linking fails with "multiple definition of"
//     (or "duplicate symbol") hwy::... errors.
//
// The functions have hidden visibility (local patch 0002), so the copy in your
// package is private to it and cannot clash with the copy in another package
// loaded in the same R session.
//
// Packages that only use static dispatch (no HWY_DYNAMIC_DISPATCH, no
// HWY_EXPORT) can skip this file and define HWY_HEADER_ONLY instead; see the
// package vignette.

#if defined(HIGHWAY_IMPL_H_INCLUDED_)
#error "highway/impl.h was included twice in the same translation unit. Include it exactly once, in a single .cpp file of your package (use_highway() creates src/highway-impl.cpp for this)."
#endif
#define HIGHWAY_IMPL_H_INCLUDED_

#if defined(HWY_HEADER_ONLY)
#error "highway/impl.h provides the compiled runtime for dynamic dispatch and cannot be combined with HWY_HEADER_ONLY (static dispatch only). Remove one of them."
#endif

#if defined(HWY_TARGET_INCLUDE)
#error "highway/impl.h must be the only Highway code in its translation unit, but HWY_TARGET_INCLUDE is already defined. Put your kernels in a separate .cpp file."
#endif

// per_target.cc first: it enables all attainable targets for its own
// re-inclusion through hwy/foreach_target.h, which must happen before any
// other file includes hwy/detect_targets.h.
#include "hwy/per_target.cc"
#include "hwy/targets.cc"
#include "hwy/abort.cc"
#include "hwy/aligned_allocator.cc"
#include "hwy/contrib/image/image.cc"
