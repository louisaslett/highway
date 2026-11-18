# Runs the demo kernels on every target that is both compiled into this
# package and supported by this CPU, not only the best one, by restricting
# dispatch with Highway's SetSupportedTargetsForTest().

usable <- intersect(compiled_targets(), supported_targets())
cat("testing targets:", paste(usable, collapse = " "), "\n")
expect_true(length(usable) >= 1L)

ulp <- highway:::.ulp_diff
x <- c(seq(-745, 709, length.out = 997), 0, -0, NA, NaN, Inf, -Inf,
       5e-324, 1e-300, 1e300)
pos <- c(10^seq(-310, 308, length.out = 499), 0, -1, NA, NaN, Inf, 5e-324)
spill_x <- as.double((seq_len(1003) * 7L) %% 13L)

reference <- list(exp = exp(x), log = suppressWarnings(log(pos)))

for (target in usable) {
  res <- highway:::.with_target(target, list(
    dispatched = dispatched_target(),
    lanes = highway:::.lanes_f64(),
    exp = hwy_demo_exp(x),
    log = hwy_demo_log(pos),
    sum = hwy_demo_sum(sin(seq_len(1001))),
    sum_na = hwy_demo_sum(c(1, NA, 3)),
    spill = highway:::.test_spill(spill_x)
  ))
  expect_false(is.null(res), info = target)
  # The forced target is the one that actually ran.
  expect_identical(res$dispatched, target, info = target)
  e <- reference$exp
  expect_identical(is.na(res$exp), is.na(e), info = target)
  expect_identical(is.nan(res$exp), is.nan(e), info = target)
  expect_true(all(ulp(res$exp[is.finite(e)], e[is.finite(e)]) <= 2), info = target)
  expect_identical(res$exp[!is.finite(e)], e[!is.finite(e)], info = target)
  l <- reference$log
  expect_identical(is.nan(res$log), is.nan(l), info = target)
  expect_true(all(ulp(res$log[is.finite(l)], l[is.finite(l)]) <= 5), info = target)
  expect_identical(res$log[!is.finite(l)], l[!is.finite(l)], info = target)
  s <- sin(seq_len(1001))
  expect_true(abs(res$sum - sum(s)) <= 1001 * .Machine$double.eps * sum(abs(s)),
              info = target)
  expect_identical(res$sum_na, NA_real_, info = target)
  # 40 live vectors force stack spills; with exact arithmetic the SIMD and
  # scalar checksums must agree bit for bit.
  expect_identical(res$spill[1], res$spill[2], info = target)
}

# Dispatch is back to normal afterwards.
expect_identical(dispatched_target(), usable[1L])

# Targets that are not compiled or not supported cannot be forced.
expect_null(highway:::.with_target("NO_SUCH_TARGET", 1))
