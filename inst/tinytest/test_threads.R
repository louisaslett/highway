# Thread safety, within CRAN's limit of two threads.

x <- seq(-30, 30, length.out = 20011)
ref <- hwy_demo_exp(x)

# Warm: dispatch already initialised.
m <- highway:::.test_threads(x, 2L, cold = FALSE)
expect_identical(dim(m), c(length(x), 2L))
expect_identical(m[, 1], ref)
expect_identical(m[, 2], ref)

# Cold start: reset Highway's chosen target so that both threads make the
# first dispatch at the same moment. Repeat to give races a chance to show.
for (i in 1:25) {
  m <- highway:::.test_threads(x, 2L, cold = TRUE)
  expect_identical(m[, 1], ref)
  expect_identical(m[, 2], ref)
}
expect_identical(dispatched_target(), intersect(compiled_targets(), supported_targets())[1L])

# One thread is fine too; more than two is refused.
expect_identical(highway:::.test_threads(x, 1L)[, 1], ref)
expect_error(highway:::.test_threads(x, 3L), "CRAN")
