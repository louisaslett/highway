# Runs the tests in inst/tinytest if tinytest is installed (it is only in
# Suggests). After installation, users can run the same tests with
# tinytest::test_package("highway").
if (requireNamespace("tinytest", quietly = TRUE)) {
  cat("highway", format(utils::packageVersion("highway")),
      "bundling Highway", format(highway::highway_version()), "\n")
  cat("supported targets: ", paste(highway::supported_targets(), collapse = " "), "\n")
  cat("compiled targets:  ", paste(highway::compiled_targets(), collapse = " "), "\n")
  cat("dispatched target: ", highway::dispatched_target(), "\n")
  tinytest::test_package("highway")
}
