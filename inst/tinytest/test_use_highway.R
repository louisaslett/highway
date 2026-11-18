# use_highway() edits only the given package directory, never overwrites
# without consent, and is idempotent.

new_pkg <- function(desc = c("Package: toy.pkg", "Version: 0.1", "Title: Toy",
                             "License: MIT")) {
  dir <- tempfile("pkg")
  dir.create(dir)
  writeLines(desc, file.path(dir, "DESCRIPTION"))
  dir
}
quiet <- function(expr) suppressMessages(expr)

# Fresh package, C++.
p <- new_pkg()
res <- quiet(use_highway(p))
expect_identical(unname(res[["DESCRIPTION"]]), "modified")
expect_identical(unname(read.dcf(file.path(p, "DESCRIPTION"), "LinkingTo")[1, 1]), "highway")
expect_true(all(c("highway-impl.cpp", "highway-kernels.cpp", "highway-kernels.h",
                  "Makevars", "Makevars.win") %in% list.files(file.path(p, "src"))))
impl <- readLines(file.path(p, "src", "highway-impl.cpp"))
expect_identical(sum(grepl("#include \"highway/impl.h\"", impl, fixed = TRUE)), 1L)
mk <- readLines(file.path(p, "src", "Makevars"))
expect_true(any(grepl("^PKG_CPPFLAGS = -I\\.$", mk)))
kern <- readLines(file.path(p, "src", "highway-kernels.cpp"))
expect_true(any(grepl("namespace toy_pkg_hwy", kern, fixed = TRUE)))
expect_true(any(grepl("HWY_TARGET_INCLUDE \"highway-kernels.cpp\"", kern, fixed = TRUE)))
expect_false(any(grepl("{{", kern, fixed = TRUE)))
# No architecture-specific flags anywhere.
expect_false(any(grepl("-m(avx|sse|arch|tune|fma)", unlist(lapply(
  list.files(file.path(p, "src"), full.names = TRUE), readLines)))))

# Second run changes nothing.
before <- lapply(list.files(p, recursive = TRUE, full.names = TRUE), readLines)
res2 <- quiet(use_highway(p))
expect_true(all(res2 == "unchanged"))
after <- lapply(list.files(p, recursive = TRUE, full.names = TRUE), readLines)
expect_identical(after, before)

# Modified templates are not overwritten unless asked.
writeLines("// my own kernels", file.path(p, "src", "highway-kernels.cpp"))
res3 <- quiet(use_highway(p))
expect_identical(unname(res3[["src/highway-kernels.cpp"]]), "skipped")
expect_identical(readLines(file.path(p, "src", "highway-kernels.cpp")), "// my own kernels")
res4 <- quiet(use_highway(p, overwrite = TRUE))
expect_identical(unname(res4[["src/highway-kernels.cpp"]]), "overwritten")
expect_identical(readLines(file.path(p, "src", "highway-kernels.cpp")), kern)

# Existing LinkingTo and Makevars are extended, not replaced.
p2 <- new_pkg(c("Package: two", "Version: 0.1", "Title: Two", "License: MIT",
                "LinkingTo:", "    Rcpp (>= 1.0.0),", "    cpp11", "Imports: Rcpp"))
dir.create(file.path(p2, "src"))
writeLines(c("CXX_STD = CXX17", "PKG_CPPFLAGS = -I../inst/include \\",
             "  -DFOO", "PKG_LIBS = -lm"), file.path(p2, "src", "Makevars"))
quiet(use_highway(p2, kernel_template = FALSE))
d2 <- readLines(file.path(p2, "DESCRIPTION"))
expect_identical(d2[1:6], c("Package: two", "Version: 0.1", "Title: Two",
                            "License: MIT", "LinkingTo:", "    Rcpp (>= 1.0.0),"))
expect_identical(d2[7], "    cpp11, highway")
expect_identical(d2[8], "Imports: Rcpp")
mk2 <- readLines(file.path(p2, "src", "Makevars"))
expect_identical(mk2, c("CXX_STD = CXX17", "PKG_CPPFLAGS = -I../inst/include \\",
                        "  -DFOO -I.", "PKG_LIBS = -lm"))
expect_false(file.exists(file.path(p2, "src", "highway-kernels.cpp")))
# Already present: unchanged.
res5 <- quiet(use_highway(p2, kernel_template = FALSE))
expect_identical(unname(res5[["DESCRIPTION"]]), "unchanged")
expect_identical(unname(res5[["src/Makevars"]]), "unchanged")

# C packages get extern "C" kernels and a C header.
p3 <- new_pkg()
quiet(use_highway(p3, language = "c"))
h <- readLines(file.path(p3, "src", "highway-kernels.h"))
expect_true(any(grepl("extern \"C\"", h, fixed = TRUE)))
expect_true(any(grepl("int toy_pkg_hwy_add(const double* a", h, fixed = TRUE)))
expect_true(any(grepl("#include \"highway/c_status.h\"", h, fixed = TRUE)))
k3 <- readLines(file.path(p3, "src", "highway-kernels.cpp"))
expect_true(sum(grepl("highway::c_guard(", k3, fixed = TRUE)) >= 2L)

# Old C++ standards are flagged.
p4 <- new_pkg()
dir.create(file.path(p4, "src"))
writeLines("CXX_STD = CXX11", file.path(p4, "src", "Makevars"))
expect_warning(quiet(use_highway(p4, kernel_template = FALSE)), "C\\+\\+17")

# Errors.
expect_error(use_highway(tempfile("missing")), "DESCRIPTION")
expect_error(use_highway(p, overwrite = NA), "overwrite")
expect_error(use_highway(p, language = "fortran"))

unlink(c(p, p2, p3, p4), recursive = TRUE)
