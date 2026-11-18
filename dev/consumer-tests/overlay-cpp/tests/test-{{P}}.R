library({{P}})

ulp <- function(x, ref) {
  ok <- is.finite(ref) & x != ref
  e <- floor(log2(pmax(abs(ref[ok]), .Machine$double.xmin)))
  out <- numeric(length(ref))
  out[ok] <- abs(x[ok] - ref[ok]) / 2^(e - 52)
  out
}

tg <- hw_targets()
cat("{{P}} supported:", tg$supported, "\n")
cat("{{P}} compiled: ", tg$compiled, "\n")
cat("{{P}} dispatched:", tg$dispatched, "\n")
usable <- intersect(tg$compiled, tg$supported)
stopifnot(identical(tg$dispatched, usable[1]))

x <- c(seq(-745, 709, length.out = 4001), 0, -0, NA, NaN, Inf, -Inf, 5e-324)
stopifnot(identical(hw_add(1:10, 10:1), rep(11, 10)))
y <- hw_exp(x)
r <- exp(x)
stopifnot(identical(is.na(y), is.na(r)), identical(is.nan(y), is.nan(r)),
          identical(y[!is.finite(r)], r[!is.finite(r)]),
          all(ulp(y[is.finite(r)], r[is.finite(r)]) <= 2))

# Every compiled and supported target.
p <- c(10^seq(-307, 308, length.out = 2001), 0, -1, NA, Inf, 5e-324)
rl <- suppressWarnings(log(p))
for (t in usable) {
  stopifnot(hw_force_target(t))
  stopifnot(identical(hw_targets()$dispatched, t))
  yl <- hw_log(p)
  stopifnot(identical(is.nan(yl), is.nan(rl)),
            identical(yl[!is.finite(rl)], rl[!is.finite(rl)]),
            all(ulp(yl[is.finite(rl)], rl[is.finite(rl)]) <= 5))
  cat("  target", t, "OK\n")
}
stopifnot(hw_force_target(""), identical(hw_targets()$dispatched, usable[1]))

# Threads: many at once, including concurrent first dispatch.
big <- seq(-50, 50, length.out = 200003)
ref <- hw_exp(big)
for (i in 1:20) {
  stopifnot(identical(hw_exp_threads(big, 8L, cold = TRUE), ref))
}
stopifnot(identical(hw_exp_threads(big, 8L, cold = FALSE), ref))

# Aborts become R errors on the main thread and status codes on workers.
msg <- tryCatch({ hw_abort(5L); "no error" }, error = conditionMessage)
stopifnot(grepl("Highway abort: {{P}} kernel abort (code 5)", msg, fixed = TRUE))
stopifnot(identical(hw_abort_workers(2L, 4L), rep(1L, 4)))
stopifnot(identical(hw_abort_workers(0L, 4L), rep(0L, 4)))

# A custom abort handler runs (on any thread) and Highway still throws.
hw_set_abort_handler(TRUE)
n0 <- hw_handler_calls()
stopifnot(inherits(tryCatch(hw_abort(1L), error = identity), "error"))
stopifnot(identical(hw_abort_workers(1L, 2L), rep(1L, 2)))
stopifnot(hw_handler_calls() == n0 + 3L)
hw_set_abort_handler(FALSE)

# warm_up() initialises dispatch.
hw_deinit()
stopifnot(!hw_chosen_initialized())
hw_warm_up()
stopifnot(hw_chosen_initialized())
cat("{{P}}: all tests passed\n")
