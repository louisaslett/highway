# Correctness of the demo kernels against base R, on the dispatched target.
# (test_all_targets.R repeats the core checks on every usable target.)

ulp <- highway:::.ulp_diff
L <- highway:::.lanes_f64()

# Documented accuracy of Highway's Exp/Log (1 and 4 ULP), plus 1 ULP for the
# error of the C library functions base R uses.
EXP_ULP <- 2
LOG_ULP <- 5

dmax <- .Machine$double.xmax
dmin <- .Machine$double.xmin
denorm <- 5e-324

edge <- c(0, -0, denorm, -denorm, dmin, -dmin, 1e-300, -1e-300, 1, -1,
          Inf, -Inf, NaN, NA_real_, dmax, -dmax, 1e300, -1e300,
          700, 705.9, 706, 706.1, 709, 709.78, 709.79, 710, 1000,
          -700, -707.9, -708, -708.1, -709, -745, -745.2, -746, -1000)

check_exp <- function(x) {
  y <- hwy_demo_exp(x)
  r <- exp(x)
  expect_identical(is.na(y), is.na(r))
  expect_identical(is.nan(y), is.nan(r))  # NA stays NA, NaN stays NaN
  expect_identical(y[!is.finite(r)], r[!is.finite(r)])
  expect_true(all(ulp(y[is.finite(r)], r[is.finite(r)]) <= EXP_ULP))
}

check_log <- function(x) {
  y <- hwy_demo_log(x)
  r <- suppressWarnings(log(x))
  expect_identical(is.na(y), is.na(r))
  expect_identical(is.nan(y), is.nan(r))
  expect_identical(y[!is.finite(r)], r[!is.finite(r)])
  expect_true(all(ulp(y[is.finite(r)], r[is.finite(r)]) <= LOG_ULP))
}

# Edge values, alone and embedded among ordinary values at every offset, so
# that each lands in every lane position and in the tail.
check_exp(edge)
check_log(edge)
for (e in edge) {
  for (pos in seq_len(L + 1L)) {
    x <- seq(-3, 3, length.out = 2L * L + 3L)
    x[pos] <- e
    check_exp(x)
    check_log(abs(x) + (pos %% 2L))
  }
}

# Exact identities and payloads.
expect_identical(hwy_demo_exp(c(0, -0)), c(1, 1))
expect_identical(hwy_demo_log(c(1, 0, -0, Inf)), c(0, -Inf, -Inf, Inf))
expect_identical(hwy_demo_exp(c(NA, NaN, -Inf, Inf)), c(NA, NaN, 0, Inf))
expect_true(is.na(hwy_demo_exp(NA_real_)) && !is.nan(hwy_demo_exp(NA_real_)))
expect_true(is.nan(hwy_demo_log(-1)))

# Lengths 0, 1, lanes - 1, lanes, lanes + 1 and larger.
lens <- unique(c(0L, 1L, L - 1L, L, L + 1L, 2L * L + 1L, 1000L, 100003L))
for (n in lens) {
  x <- seq(-20, 20, length.out = n)
  check_exp(x)
  check_log(abs(x) + 1e-3)
  expect_identical(length(hwy_demo_exp(x)), n)
}

# Wide range of magnitudes.
x <- c(seq(-745, 709, by = 0.37), 10^seq(-310, 308, by = 0.5))
check_exp(x)
check_exp(-x)
check_log(x)

# Attributes are kept, like exp().
m <- matrix(c(0.5, 1, 2, 3), 2, dimnames = list(c("a", "b"), NULL))
expect_identical(attributes(hwy_demo_exp(m)), attributes(exp(m)))
expect_identical(names(hwy_demo_log(c(a = 1, b = 2))), c("a", "b"))
expect_identical(hwy_demo_exp(1:3), hwy_demo_exp(c(1, 2, 3)))
expect_error(hwy_demo_exp("a"), "numeric")
expect_error(hwy_demo_exp(Sys.Date()), "numeric")

# Sum: rounding may differ from base R (which uses long double where
# available); bound the difference by n * eps * sum(|x|).
for (n in lens) {
  x <- sin(seq_len(n)) * 1e3
  tol <- max(1, n) * .Machine$double.eps * sum(abs(x))
  expect_true(abs(hwy_demo_sum(x) - sum(x)) <= tol)
}
expect_identical(hwy_demo_sum(numeric()), 0)
expect_identical(hwy_demo_sum(c(1, 2, NA, 4)), sum(c(1, 2, NA, 4)))
expect_identical(hwy_demo_sum(c(1, 2, NaN, 4)), sum(c(1, 2, NaN, 4)))
expect_identical(hwy_demo_sum(c(1, NA, 3), na.rm = TRUE), 4)
expect_identical(hwy_demo_sum(c(1, Inf, 3)), Inf)
expect_identical(hwy_demo_sum(c(-Inf, Inf)), sum(c(-Inf, Inf)))
expect_identical(hwy_demo_sum(rep(1, 10 * L + 3)), 10 * L + 3)
expect_error(hwy_demo_sum(1, na.rm = NA), "na.rm")
