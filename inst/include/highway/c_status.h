/* Copyright 2026 Louis Aslett
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Part of the R package 'highway'. Status codes returned by highway::c_guard()
 * (highway/c_guard.h). This header is plain C so that the C side of an
 * extern "C" interface can include it.
 */

#ifndef HIGHWAY_C_STATUS_H_
#define HIGHWAY_C_STATUS_H_

/* Success. */
#define HIGHWAY_STATUS_OK 0
/* Highway aborted (HWY_ABORT / HWY_ASSERT): a highway::abort_error. */
#define HIGHWAY_STATUS_ABORT 1
/* Memory allocation failed (std::bad_alloc). */
#define HIGHWAY_STATUS_NO_MEMORY 2
/* Any other C++ exception derived from std::exception. */
#define HIGHWAY_STATUS_EXCEPTION 3
/* An exception not derived from std::exception. */
#define HIGHWAY_STATUS_UNKNOWN 4
/* Invalid arguments detected by the caller's own checks (not set by the guard;
 * available for kernels to report bad input without throwing). */
#define HIGHWAY_STATUS_INVALID_ARGUMENT 5

#endif /* HIGHWAY_C_STATUS_H_ */
