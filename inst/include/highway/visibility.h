// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Part of the R package 'highway'. Symbol visibility for the runtime parts of
// Highway that each consumer package compiles into its own shared object (via
// highway/impl.h). Hidden visibility keeps two consumer packages loaded in the
// same R session from binding to each other's dispatch state or handlers.

#ifndef HIGHWAY_VISIBILITY_H_
#define HIGHWAY_VISIBILITY_H_

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_WIN32) && \
    !defined(__CYGWIN__)
#define HIGHWAY_HIDDEN __attribute__((visibility("hidden")))
#else
// Windows DLLs only export what is explicitly exported; nothing to do.
#define HIGHWAY_HIDDEN
#endif

#endif  // HIGHWAY_VISIBILITY_H_
