library(consumerRcpp)
cat("consumerRcpp dispatched:", rcpp_hwy_target(), "\n")
x <- c(seq(-745, 709, length.out = 4001), 0, -0, NA, NaN, Inf, -Inf)
y <- rcpp_hwy_exp(x)
r <- exp(x)
stopifnot(identical(is.na(y), is.na(r)), identical(is.nan(y), is.nan(r)),
          identical(y[!is.finite(r)], r[!is.finite(r)]),
          isTRUE(all.equal(y, r, tolerance = 4 * .Machine$double.eps)))
msg <- tryCatch({ rcpp_hwy_abort(3L); "no error" }, error = conditionMessage)
stopifnot(grepl("consumerRcpp abort (code 3)", msg, fixed = TRUE))
rcpp_hwy_abort(0L)
cat("consumerRcpp: all tests passed\n")
