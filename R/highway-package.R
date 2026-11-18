#' highway: 'Highway' C++ headers for portable SIMD with runtime dispatch
#'
#' The package bundles the headers of Google's 'Highway' library so that
#' other packages can write portable SIMD (single instruction, multiple data)
#' kernels in C++ and use them through `LinkingTo: highway`. Kernels are
#' compiled for several instruction sets and the best one supported by the
#' running CPU is selected at run time, without CPU-specific compiler flags.
#'
#' To use it in a package, run [use_highway()] and see
#' `vignette("highway")`. The R functions in this package report on the
#' bundled library and the current CPU ([highway_version()],
#' [supported_targets()], [compiled_targets()], [dispatched_target()]) and
#' provide small demonstration kernels ([hwy_demo_exp()], [hwy_demo_log()],
#' [hwy_demo_sum()]).
#'
#' Licensing and the list of local changes to the bundled library are in
#' `system.file("COPYRIGHTS", package = "highway")`.
#'
#' @keywords internal
#' @useDynLib highway, .registration = TRUE, .fixes = "C_"
"_PACKAGE"
