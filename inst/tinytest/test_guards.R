# Highway's abort handler throws highway::abort_error; the R and C guards
# turn it into an R error or a status code instead of terminating R.

# On R's main thread, through highway/r_guard.h.
expect_silent(highway:::.test_abort_main(0L))
expect_error(highway:::.test_abort_main(7L),
             "Highway abort: highway demo kernel: abort requested \\(code 7\\)")

# On a worker std::thread, through highway::c_guard (status 1 = abort).
res <- highway:::.test_abort_worker(3L)
expect_identical(res$status, 1L)
expect_true(grepl("abort requested (code 3)", res$message, fixed = TRUE))
res <- highway:::.test_abort_worker(0L)
expect_identical(res$status, 0L)
expect_identical(res$message, "")

# Everything still works afterwards.
expect_identical(hwy_demo_exp(c(0, -Inf, NA)), c(1, 0, NA))
