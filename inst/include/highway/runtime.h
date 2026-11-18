// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Part of the R package 'highway'. Small runtime helpers for packages that use
// Highway's dynamic dispatch. Nothing here calls the R API, so every function
// may be called from any thread.

#ifndef HIGHWAY_RUNTIME_H_
#define HIGHWAY_RUNTIME_H_

#include <stddef.h>
#include <stdint.h>

#include "hwy/targets.h"
#include "highway/visibility.h"

namespace highway {

// Performs Highway's one-time CPU detection now instead of on the first
// HWY_DYNAMIC_DISPATCH call. Optional: concurrent first calls are safe without
// it. Call it from your package's R_init_<pkg>() or .onLoad() if you want the
// (sub-microsecond) detection cost out of your first kernel call. It operates
// on your package's own copy of the dispatch state (see highway/impl.h).
//
// In static-dispatch-only builds (HWY_HEADER_ONLY) there is nothing to
// initialise and this does nothing.
HIGHWAY_HIDDEN inline void warm_up() {
#if !defined(HWY_HEADER_ONLY)
  ::hwy::GetChosenTarget().Update(::hwy::SupportedTargets());
#endif
}

// Calls `fn(bit, name)` for each target bit set in `targets`, best first
// (Highway orders targets so that lower bits are better). Useful for turning
// hwy::SupportedTargets() or HWY_TARGETS into names.
template <class Fn>
HIGHWAY_HIDDEN inline void for_each_target(int64_t targets, Fn&& fn) {
  for (int bit = 0; bit < 63; ++bit) {
    const int64_t t = static_cast<int64_t>(int64_t{1} << bit);
    if ((targets & t) != 0) fn(t, ::hwy::TargetName(t));
  }
}

// Number of targets set in `targets`.
HIGHWAY_HIDDEN inline size_t count_targets(int64_t targets) {
  size_t n = 0;
  for_each_target(targets, [&n](int64_t, const char*) { ++n; });
  return n;
}

}  // namespace highway

#endif  // HIGHWAY_RUNTIME_H_
