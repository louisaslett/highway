library({{P}})
x <- c(seq(-745, 709, length.out = 4001), 0, -0, NA, NaN, Inf, -Inf)
y <- hw_exp(x)
r <- exp(x)
stopifnot(identical(is.na(y), is.na(r)), identical(is.nan(y), is.nan(r)),
          identical(y[!is.finite(r)], r[!is.finite(r)]),
          isTRUE(all.equal(y, r, tolerance = 4 * .Machine$double.eps)))
stopifnot(identical(hw_add(1:5, 5:1), rep(6, 5)))
res <- hw_abort(9L)
stopifnot(identical(res[[1]], 1L),
          grepl("C-interface abort (code 9)", res[[2]], fixed = TRUE))
stopifnot(identical(hw_abort(0L), list(0L, "")))
stopifnot(identical(hw_null_args(), 5L))  # HIGHWAY_STATUS_INVALID_ARGUMENT
cat("{{P}}: all tests passed\n")
