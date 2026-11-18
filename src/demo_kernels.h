// Copyright 2026 Louis Aslett
// SPDX-License-Identifier: BSD-3-Clause
//
// Dispatch entry points for the package's demo and test kernels, defined in
// demo_kernels.cpp. Plain C++ functions: no R API, callable from any thread.

#ifndef HIGHWAY_PKG_DEMO_KERNELS_H_
#define HIGHWAY_PKG_DEMO_KERNELS_H_

#include <stddef.h>
#include <stdint.h>

namespace highway_demo {

// out[i] = exp(in[i]); identical to R's exp() for all inputs.
void Exp(const double* in, double* out, size_t n);
// out[i] = log(in[i]); identical to R's log() for all inputs.
void Log(const double* in, double* out, size_t n);
// Sum of in[0, n), skipping NaN/NA if na_rm.
double Sum(const double* in, size_t n, bool na_rm);

// Target bit (HWY_*) that dynamic dispatch selects in this translation unit.
int64_t DispatchedTarget();
// Number of double lanes of the dispatched target.
size_t LanesF64();
// HWY_TARGETS of demo_kernels.cpp: the targets compiled for the kernels.
int64_t CompiledTargets();

// Test kernels.
// Holds 40 vectors live at once to force register spills on every target;
// returns a checksum that tests compare with a scalar reference.
double SpillChecksum(const double* in, size_t n);
double SpillChecksumScalar(const double* in, size_t n);
// Calls HWY_ABORT from inside a dispatched kernel.
void AbortInKernel(int code);

}  // namespace highway_demo

#endif  // HIGHWAY_PKG_DEMO_KERNELS_H_
