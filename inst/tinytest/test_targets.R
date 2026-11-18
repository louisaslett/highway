# Version and target reporting.

v <- highway_version()
expect_true(inherits(v, "numeric_version"))
pkg_v <- unclass(utils::packageVersion("highway"))[[1]]
expect_identical(unclass(v)[[1]], pkg_v[1:3])

sup <- supported_targets()
comp <- compiled_targets()
disp <- dispatched_target()
expect_true(is.character(sup) && length(sup) >= 1L)
expect_true(is.character(comp) && length(comp) >= 1L)
expect_true(is.character(disp) && length(disp) == 1L)
expect_false(anyDuplicated(sup) > 0)
expect_false(anyDuplicated(comp) > 0)

# The dispatched target is the best target that is compiled and supported.
usable <- intersect(comp, sup)
expect_true(length(usable) >= 1L)
expect_identical(disp, usable[1L])

# Portable fallbacks are always supported.
expect_true(all(c("EMU128", "SCALAR") %in% sup))

arch <- R.version$arch
if (arch %in% c("x86_64", "amd64")) {
  # Runtime dispatch is in use: several x86 targets are compiled in.
  expect_true(any(c("SSE2", "SSSE3") %in% comp))
  expect_true(all(c("SSE4", "AVX2", "AVX3") %in% comp))
  expect_true(any(c("SSE2", "SSSE3") %in% sup))
} else if (arch %in% c("aarch64", "arm64")) {
  expect_true(any(c("NEON", "NEON_WITHOUT_AES") %in% comp))
  expect_true(any(c("NEON", "NEON_WITHOUT_AES") %in% sup))
} else {
  # Architectures without bundled SIMD op headers use the portable target.
  expect_true(any(c("EMU128", "SCALAR") %in% comp))
}

# The lane count matches the dispatched target's vector width.
lanes <- highway:::.lanes_f64()
expect_true(lanes >= 1L)
if (disp %in% c("AVX3", "AVX3_DL", "AVX3_ZEN4", "AVX3_SPR", "AVX10_2")) {
  expect_equal(lanes, 8L)
} else if (disp == "AVX2") {
  expect_equal(lanes, 4L)
} else if (disp %in% c("SSE2", "SSSE3", "SSE4", "NEON", "NEON_WITHOUT_AES",
                       "NEON_BF16", "EMU128")) {
  expect_equal(lanes, 2L)
}
