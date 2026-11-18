#' Version of the bundled Highway library
#'
#' Reports the version of the 'Highway' headers bundled in this package, as
#' compiled into it. The package version has the form
#' `<Highway version>-<packaging revision>`, so `1.4.0-1` is the first release
#' of this package bundling Highway 1.4.0.
#'
#' @return A [numeric_version] such as `1.4.0`.
#' @export
#' @examples
#' highway_version()
#' highway_version() >= "1.4.0"
highway_version <- function() {
  numeric_version(paste(.Call(C_hwy_version), collapse = "."))
}

#' SIMD targets: supported, compiled and dispatched
#'
#' Highway calls each instruction set it can generate code for a *target*,
#' for example `"SSE2"`, `"AVX2"`, `"AVX3"` (AVX-512) or `"AVX3_SPR"` on
#' x86-64 and `"NEON"` or `"SVE2"` on arm64. `"EMU128"` and `"SCALAR"` are
#' portable fallbacks that need no SIMD instructions.
#'
#' * `supported_targets()` lists the targets the current CPU and operating
#'   system can run.
#' * `compiled_targets()` lists the targets compiled into this package's own
#'   demonstration kernels. A package that uses Highway compiles its own set,
#'   which depends on its compiler and settings.
#' * `dispatched_target()` gives the target this package's kernels actually
#'   use on this machine: the best one that is both compiled and supported.
#'
#' Targets are listed best first.
#'
#' @return A character vector (length one for `dispatched_target()`).
#' @export
#' @examples
#' supported_targets()
#' compiled_targets()
#' dispatched_target()
supported_targets <- function() {
  .Call(C_hwy_supported_targets)
}

#' @rdname supported_targets
#' @export
compiled_targets <- function() {
  .Call(C_hwy_compiled_targets)
}

#' @rdname supported_targets
#' @export
dispatched_target <- function() {
  .Call(C_hwy_dispatched_target)
}
