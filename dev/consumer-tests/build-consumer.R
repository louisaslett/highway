# Builds a consumer test package from scratch with highway::use_highway() and
# then adds its test code from overlay-cpp/ or overlay-c/ ({{P}} in file names
# and contents becomes the package name).
#
# Usage: Rscript dev/consumer-tests/build-consumer.R <name> <outdir>
#   <name>: consumerA (C++, lean targets, default visibility)
#           consumerB (C++, all default targets, $(CXX_VISIBILITY))
#           consumerC (C, via use_highway(language = "c"))

args <- commandArgs(trailingOnly = TRUE)
stopifnot(length(args) == 2L)
name <- args[[1]]
outdir <- normalizePath(args[[2]], mustWork = TRUE)
script <- sub("^--file=", "", grep("^--file=", commandArgs(FALSE), value = TRUE))
here <- dirname(normalizePath(script))

cfg <- switch(name,
  consumerA = list(language = "cpp", lean = TRUE, visibility = FALSE),
  consumerB = list(language = "cpp", lean = FALSE, visibility = TRUE),
  consumerC = list(language = "c", lean = TRUE, visibility = FALSE),
  stop("unknown consumer '", name, "'"))

pkg <- file.path(outdir, name)
unlink(pkg, recursive = TRUE)
dir.create(pkg)
writeLines(c(
  paste0("Package: ", name),
  "Type: Package",
  "Title: Test Consumer of the 'highway' Package",
  "Version: 0.0.1",
  "Authors@R: person(\"Louis\", \"Aslett\", role = c(\"aut\", \"cre\"),",
  "    email = \"louis.aslett@durham.ac.uk\")",
  paste0("Description: Integration test for 'highway': ",
         if (cfg$language == "c") "a package written in C calling 'SIMD' kernels"
         else "'SIMD' kernels with runtime dispatch",
         " set up with use_highway(). Not for CRAN."),
  "License: BSD_3_clause + file LICENSE",
  "Encoding: UTF-8"), file.path(pkg, "DESCRIPTION"))
writeLines(c("YEAR: 2026", "COPYRIGHT HOLDER: highway authors",
             "ORGANIZATION: highway authors"), file.path(pkg, "LICENSE"))

# 1. The step under test: set the package up from scratch.
res <- highway::use_highway(pkg, language = cfg$language)
print(res)
stopifnot(all(res %in% c("created", "modified")))

# 2. Variations a user might make.
kern <- file.path(pkg, "src", "highway-kernels.cpp")
if (!cfg$lean) {
  k <- readLines(kern)
  writeLines(k[k != "#define HIGHWAY_TARGETS_LEAN"], kern)
}
if (cfg$visibility) {
  for (mk in c("Makevars", "Makevars.win")) {
    f <- file.path(pkg, "src", mk)
    writeLines(c(readLines(f), "PKG_CXXFLAGS = $(CXX_VISIBILITY)"), f)
  }
}

# 3. Test code.
overlay <- file.path(here, if (cfg$language == "c") "overlay-c" else "overlay-cpp")
files <- list.files(overlay, recursive = TRUE, all.files = TRUE)
for (f in files) {
  dest <- file.path(pkg, gsub("{{P}}", name, f, fixed = TRUE))
  dir.create(dirname(dest), recursive = TRUE, showWarnings = FALSE)
  txt <- gsub("{{P}}", name, readLines(file.path(overlay, f)), fixed = TRUE)
  writeLines(txt, dest)
}
cat("Built", pkg, "\n")
