# Internal test hooks, used by inst/tinytest. Not exported.

# Lanes of a double vector on the dispatched target.
.lanes_f64 <- function() .Call(C_hwy_lanes_f64)

# Evaluates `expr` with dynamic dispatch restricted to `target` (a name from
# compiled_targets() that is also in supported_targets()). Returns the value
# of `expr`, or NULL if the target cannot be forced. Uses Highway's
# SetSupportedTargetsForTest(), which is not thread-safe: only for tests.
.with_target <- function(target, expr) {
  if (!isTRUE(.Call(C_hwy_test_force_target, target))) return(NULL)
  on.exit(.Call(C_hwy_test_force_target, ""), add = TRUE)
  force(expr)
}

.test_spill <- function(x) .Call(C_hwy_test_spill, as.double(x))

.test_abort_main <- function(code) .Call(C_hwy_test_abort_main, as.integer(code))

.test_abort_worker <- function(code) .Call(C_hwy_test_abort_worker, as.integer(code))

.test_threads <- function(x, nthreads = 2L, cold = FALSE) {
  .Call(C_hwy_test_threads, as.double(x), as.integer(nthreads), isTRUE(cold))
}

# Distance between doubles in units in the last place (ULP) of `ref`, for
# finite values; 0 where both are identical (including both infinite).
.ulp_diff <- function(x, ref) {
  out <- numeric(length(ref))
  same <- (x == ref) %in% TRUE
  fin <- is.finite(ref) & is.finite(x) & !same
  e <- floor(log2(pmax(abs(ref[fin]), .Machine$double.xmin)))
  out[fin] <- abs(x[fin] - ref[fin]) / 2^(e - 52)
  out[!same & !fin] <- Inf
  out
}
