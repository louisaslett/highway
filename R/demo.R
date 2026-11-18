#' Demonstration SIMD kernels
#'
#' Small kernels written with Highway and compiled into this package, using
#' runtime dispatch (see [dispatched_target()]). They exist to show that the
#' bundled headers work on the current platform and to give the package
#' vignette real output; their C++ source is in the package's `src/`
#' directory.
#'
#' `hwy_demo_exp()` and `hwy_demo_log()` return exactly what [exp()] and
#' [log()] return for `NA`, `NaN`, `Inf`, `-Inf`, `0`, `-0`, negative numbers,
#' subnormal numbers, and results that overflow or underflow. Such elements
#' are computed as base R computes them (with the C library, returning `NA`
#' and `NaN` inputs unchanged); all others use Highway's
#' SIMD `Exp()` and `Log()`, which are accurate to 1 and 4 units in the last
#' place respectively, so they can differ from base R in the last bits.
#'
#' `hwy_demo_sum()` adds the elements with SIMD instructions. Base R adds in
#' extended precision where the platform has it, so results can differ from
#' [sum()] by rounding. If `x` contains `NA`, `NaN` or infinite values the
#' result is computed exactly as [sum()] computes it.
#'
#' @param x A numeric vector. Integer and logical vectors are converted to
#'   double.
#' @param na.rm Logical: should missing values (`NA` and `NaN`) be removed?
#' @return `hwy_demo_exp()` and `hwy_demo_log()` return a double vector with
#'   the attributes of `x`; `hwy_demo_sum()` returns a single double.
#' @export
#' @examples
#' x <- c(-1, 0, 0.5, 1, 2, NA, Inf)
#' hwy_demo_exp(x)
#' all.equal(hwy_demo_exp(x), exp(x))
#'
#' hwy_demo_log(c(1, exp(1), 10, 0, -1, NA))
#'
#' y <- seq(0, 1, length.out = 1001)
#' hwy_demo_sum(y)
#' all.equal(hwy_demo_sum(y), sum(y))
hwy_demo_exp <- function(x) {
  x <- as_double_vector(x)
  keep_attributes(.Call(C_hwy_demo_exp, x), x)
}

#' @rdname hwy_demo_exp
#' @export
hwy_demo_log <- function(x) {
  x <- as_double_vector(x)
  keep_attributes(.Call(C_hwy_demo_log, x), x)
}

#' @rdname hwy_demo_exp
#' @export
hwy_demo_sum <- function(x, na.rm = FALSE) {
  if (!is.logical(na.rm) || length(na.rm) != 1L || is.na(na.rm)) {
    stop("'na.rm' must be TRUE or FALSE", call. = FALSE)
  }
  .Call(C_hwy_demo_sum, as_double_vector(x), na.rm)
}

as_double_vector <- function(x) {
  if (!(is.numeric(x) || is.logical(x)) || is.object(x)) {
    stop("'x' must be a numeric vector", call. = FALSE)
  }
  if (!is.double(x)) {
    a <- attributes(x)
    x <- as.double(x)
    attributes(x) <- a
  }
  x
}

keep_attributes <- function(y, x) {
  attributes(y) <- attributes(x)
  y
}
