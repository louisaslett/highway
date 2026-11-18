#' Set up a package to use Highway
#'
#' Prepares the package at `path` to use the bundled 'Highway' headers with
#' runtime dispatch, in the style of `cpp11::use_cpp11()` and
#' `usethis::use_rcpp()`. It
#'
#' * adds `highway` to the `LinkingTo` field of `DESCRIPTION`;
#' * creates `src/highway-impl.cpp`, the one file that includes
#'   `"highway/impl.h"` (Highway's runtime, compiled once per package);
#' * adds `-I.` to `PKG_CPPFLAGS` in `src/Makevars` and `src/Makevars.win`
#'   (creating them if needed), so that `hwy/foreach_target.h` can re-include
#'   the kernel file named in `HWY_TARGET_INCLUDE`;
#' * if `kernel_template = TRUE`, writes `src/highway-kernels.cpp` with two
#'   example dispatched kernels and `src/highway-kernels.h` declaring them.
#'
#' With `language = "cpp"` the kernels are declared as C++ functions, to be
#' called from Rcpp, cpp11 or `.Call` code. With `language = "c"`, for
#' packages whose code is otherwise written in C, they are declared as plain
#' `extern "C"` functions that take raw pointers and lengths, return an
#' integer status code (see `highway/c_status.h`) and never let a C++
#' exception escape; the header can be included from `.c` files. R compiles
#' and links packages that mix `.c` and `.cpp` files without further
#' settings.
#'
#' Existing files are never replaced unless `overwrite = TRUE`. Changes to an
#' existing `DESCRIPTION` or `Makevars` only add to them. Nothing outside
#' `path` is touched. Each action is reported with [message()].
#'
#' The function does not register native routines or edit `NAMESPACE`; it
#' prints what remains to be done.
#'
#' @param path Path to the root of an R package source directory.
#' @param language `"cpp"` or `"c"`: the language of the code that will call
#'   the kernels. See Details.
#' @param kernel_template Logical: write the example kernel files?
#' @param overwrite Logical: replace existing template files? Files that
#'   already have the template's content are left alone either way.
#' @return Invisibly, a named character vector with one element per file
#'   considered, giving the action taken: `"created"`, `"modified"`,
#'   `"overwritten"`, `"unchanged"` or `"skipped"` (exists, not overwritten).
#'   Names are paths relative to `path`.
#' @export
#' @examples
#' # Set up a throwaway package in a temporary directory.
#' pkg <- file.path(tempdir(), "mysimd")
#' dir.create(pkg)
#' writeLines(c("Package: mysimd", "Version: 0.0.1",
#'              "Title: Example", "License: MIT"),
#'            file.path(pkg, "DESCRIPTION"))
#' use_highway(pkg)
#' readLines(file.path(pkg, "DESCRIPTION"))
#' list.files(file.path(pkg, "src"))
#' unlink(pkg, recursive = TRUE)
use_highway <- function(path = ".", language = c("cpp", "c"),
                        kernel_template = TRUE, overwrite = FALSE) {
  language <- match.arg(language)
  check_flag(kernel_template, "kernel_template")
  check_flag(overwrite, "overwrite")
  if (!is.character(path) || length(path) != 1L || is.na(path)) {
    stop("'path' must be a single path", call. = FALSE)
  }
  desc_file <- file.path(path, "DESCRIPTION")
  if (!file.exists(desc_file)) {
    stop("no DESCRIPTION file in '", path, "': is it an R package?",
         call. = FALSE)
  }
  pkg <- unname(read.dcf(desc_file, fields = "Package")[1L, 1L])
  if (is.na(pkg)) stop("DESCRIPTION has no Package field", call. = FALSE)
  prefix <- gsub("[^A-Za-z0-9_]", "_", pkg)

  src <- file.path(path, "src")
  if (file.exists(file.path(src, "Makefile")) ||
      file.exists(file.path(src, "Makefile.win"))) {
    warning("src/Makefile found: add '-I.' and the 'highway' include ",
            "directory to your compiler flags by hand", call. = FALSE)
  }

  result <- character()
  record <- function(file, action) {
    result[[file]] <<- action
    message("* ", file, ": ", action_text(action))
  }

  record("DESCRIPTION", add_linking_to(desc_file))

  if (!dir.exists(src)) dir.create(src)
  record("src/highway-impl.cpp",
         write_template("highway-impl.cpp", file.path(src, "highway-impl.cpp"),
                        pkg, prefix, overwrite))

  for (mk in makevars_targets(src)) {
    record(file.path("src", mk), add_include_dot(file.path(src, mk)))
  }

  if (kernel_template) {
    stem <- if (language == "c") "highway-kernels-c" else "highway-kernels-cpp"
    for (ext in c("cpp", "h")) {
      rel <- paste0("src/highway-kernels.", ext)
      record(rel, write_template(paste0(stem, ".", ext), file.path(path, rel),
                                 pkg, prefix, overwrite))
    }
  }

  warn_old_cxx_standard(path)

  message("\nNext steps:")
  message("* Make sure NAMESPACE loads the compiled code, e.g. with ",
          "'useDynLib(", pkg, ", .registration = TRUE)'\n",
          "  (roxygen2: #' @useDynLib ", pkg, ", .registration = TRUE).")
  if (kernel_template) {
    if (language == "c") {
      message("* Call the kernels declared in src/highway-kernels.h from ",
              "your .c files and check their status code.")
    } else {
      message("* Call the kernels declared in src/highway-kernels.h from ",
              "your Rcpp, cpp11 or .Call code (for plain .Call, see ",
              "highway/r_guard.h).")
    }
  }
  message("* See vignette(\"highway\", package = \"highway\") for kernels, ",
          "dispatch and CRAN advice.")
  invisible(result)
}

action_text <- function(action) {
  switch(action,
    created = "created",
    modified = "updated",
    overwritten = "overwritten",
    unchanged = "already set up, left unchanged",
    skipped = "exists with different content; not overwritten (use overwrite = TRUE)",
    action
  )
}

check_flag <- function(x, name) {
  if (!is.logical(x) || length(x) != 1L || is.na(x)) {
    stop("'", name, "' must be TRUE or FALSE", call. = FALSE)
  }
}

# Makevars variants to update: always Makevars and Makevars.win; also
# Makevars.ucrt if the package has one. If a Makevars.in template exists
# (configure generates Makevars from it), update the template instead.
makevars_targets <- function(src) {
  unix <- if (file.exists(file.path(src, "Makevars.in"))) "Makevars.in" else "Makevars"
  win <- if (file.exists(file.path(src, "Makevars.win.in"))) "Makevars.win.in" else "Makevars.win"
  out <- c(unix, win)
  if (file.exists(file.path(src, "Makevars.ucrt"))) out <- c(out, "Makevars.ucrt")
  out
}

template_text <- function(name, pkg, prefix) {
  file <- system.file("templates", name, package = "highway", mustWork = TRUE)
  txt <- readLines(file, warn = FALSE)
  txt <- gsub("{{pkg}}", pkg, txt, fixed = TRUE)
  txt <- gsub("{{prefix}}", prefix, txt, fixed = TRUE)
  gsub("{{PREFIX}}", toupper(prefix), txt, fixed = TRUE)
}

write_template <- function(name, dest, pkg, prefix, overwrite) {
  txt <- template_text(name, pkg, prefix)
  if (file.exists(dest)) {
    if (identical(readLines(dest, warn = FALSE), txt)) return("unchanged")
    if (!overwrite) return("skipped")
    writeLines(txt, dest)
    return("overwritten")
  }
  writeLines(txt, dest)
  "created"
}

# Adds 'highway' to LinkingTo, keeping every other byte of DESCRIPTION.
add_linking_to <- function(desc_file) {
  lines <- readLines(desc_file, warn = FALSE, encoding = "bytes")
  start <- grep("^LinkingTo:", lines, useBytes = TRUE)
  if (length(start)) {
    start <- start[1L]
    end <- start
    while (end < length(lines) && grepl("^[[:space:]]", lines[end + 1L],
                                        useBytes = TRUE)) {
      end <- end + 1L
    }
    value <- sub("^LinkingTo:", "", paste(lines[start:end], collapse = " "),
                 useBytes = TRUE)
    deps <- trimws(sub("\\(.*$", "", strsplit(value, ",", fixed = TRUE)[[1L]],
                       useBytes = TRUE))
    if ("highway" %in% deps) return("unchanged")
    sep <- if (any(nzchar(deps))) ", " else " "
    lines[end] <- paste0(sub("[[:space:]]+$", "", lines[end], useBytes = TRUE),
                         sep, "highway")
  } else {
    last <- max(c(0L, which(nzchar(trimws(lines)))))
    lines <- append(lines, "LinkingTo: highway", after = last)
  }
  writeLines(lines, desc_file, useBytes = TRUE)
  "modified"
}

# Ensures PKG_CPPFLAGS contains -I. (as a separate token).
add_include_dot <- function(file) {
  header <- c(
    "# -I. lets hwy/foreach_target.h find the kernel file named in",
    "# HWY_TARGET_INCLUDE (added by highway::use_highway()).")
  if (!file.exists(file)) {
    writeLines(c(header, "PKG_CPPFLAGS = -I."), file)
    return("created")
  }
  lines <- readLines(file, warn = FALSE)
  assign <- grep("^[[:space:]]*PKG_CPPFLAGS[[:space:]]*[+:?]?=", lines)
  # The extent of each (possibly backslash-continued) assignment.
  extent <- function(i) {
    j <- i
    while (j < length(lines) && grepl("\\\\[[:space:]]*$", lines[j])) j <- j + 1L
    i:j
  }
  has_dot <- function(i) {
    any(grepl("(^|[[:space:]=])-I\\.([[:space:]]|$)", lines[extent(i)]))
  }
  if (any(vapply(assign, has_dot, logical(1)))) return("unchanged")
  if (length(assign)) {
    j <- max(extent(assign[length(assign)]))
    lines[j] <- paste(sub("[[:space:]]+$", "", lines[j]), "-I.")
  } else {
    lines <- c(lines, header, "PKG_CPPFLAGS = -I.")
  }
  writeLines(lines, file)
  "modified"
}

# Highway needs C++17 or later; R >= 4.4 defaults to it, but a package can
# request an older standard explicitly.
warn_old_cxx_standard <- function(path) {
  old <- "CXX(98|11|14)\\b"
  files <- file.path(path, "src", c("Makevars", "Makevars.win", "Makevars.ucrt",
                                    "Makevars.in"))
  hits <- vapply(files, function(f) {
    file.exists(f) && any(grepl(paste0("^[[:space:]]*CXX_STD[[:space:]]*=[[:space:]]*", old),
                                readLines(f, warn = FALSE)))
  }, logical(1))
  sysreq <- read.dcf(file.path(path, "DESCRIPTION"), fields = "SystemRequirements")[1L, 1L]
  if (any(hits) || (!is.na(sysreq) && grepl("C\\+\\+ *(98|11|14)\\b", sysreq))) {
    warning("the package requests a C++ standard older than C++17 ",
            "(CXX_STD or SystemRequirements); Highway needs C++17 or later. ",
            "Remove the setting to use R's default.", call. = FALSE)
  }
}
