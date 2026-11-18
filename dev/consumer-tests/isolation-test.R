# Two consumer packages, each with its own copy of Highway's runtime
# (highway/impl.h), loaded into one R session. Checks that dispatch state and
# abort handlers do not leak between them, including when one of them is
# loaded with RTLD_GLOBAL (dyn.load(local = FALSE)), which is when
# un-hidden symbols could interpose.
#
# Usage: Rscript dev/consumer-tests/isolation-test.R <mode>
#   mode = "normal" | "B-global" | "A-global"

mode <- commandArgs(trailingOnly = TRUE)[1]
stopifnot(mode %in% c("normal", "B-global", "A-global"))
cat("isolation test, mode", mode, "\n")

# Wrap a package as a list of functions calling its registered routines, so
# that the globally loaded one does not need its namespace (loading the
# namespace would re-load its DLL locally).
via_dll <- function(pkg, local) {
  so <- system.file("libs", paste0(pkg, .Platform$dynlib.ext), package = pkg)
  if (!nzchar(so)) {
    so <- Sys.glob(file.path(system.file("libs", package = pkg), "*",
                             paste0(pkg, .Platform$dynlib.ext)))[1]
  }
  dll <- dyn.load(so, local = local)
  r <- getDLLRegisteredRoutines(dll)$.Call
  f <- function(nm) r[[paste0(pkg, "_", nm)]]
  list(
    targets = function() .Call(f("targets")),
    force = function(t) .Call(f("force_target"), t),
    init = function() .Call(f("chosen_initialized")),
    deinit = function() .Call(f("deinit")),
    warm = function() .Call(f("warm_up")),
    handler = function(on) .Call(f("set_abort_handler"), on),
    calls = function() .Call(f("handler_calls")),
    abort = function(code) .Call(f("abort"), as.integer(code)),
    exp = function(x) .Call(f("exp"), as.double(x))
  )
}
via_ns <- function(pkg) {
  ns <- asNamespace(pkg)
  list(
    targets = ns$hw_targets, force = ns$hw_force_target,
    init = ns$hw_chosen_initialized, deinit = ns$hw_deinit,
    warm = ns$hw_warm_up, handler = ns$hw_set_abort_handler,
    calls = ns$hw_handler_calls, abort = ns$hw_abort, exp = ns$hw_exp
  )
}

if (mode == "normal") {
  A <- via_ns("consumerA")
  B <- via_ns("consumerB")
} else if (mode == "B-global") {
  B <- via_dll("consumerB", local = FALSE)
  A <- via_ns("consumerA")
} else {
  A <- via_dll("consumerA", local = FALSE)
  B <- via_ns("consumerB")
}
print(getLoadedDLLs()[c("consumerA", "consumerB")])

# 1. Chosen-target state is separate.
A$warm(); B$warm()
stopifnot(A$init(), B$init())
B$deinit()
stopifnot(A$init(), !B$init())
A$deinit()
stopifnot(!A$init(), !B$init())
# targets() dispatches through the multi-target test kernels (with a single
# compiled target Highway uses static dispatch and never touches this state).
ta <- A$targets()
stopifnot(A$init(), !B$init())  # A's first dispatch initialised only A
tb <- B$targets()
stopifnot(A$init(), B$init())
x <- seq(-5, 5, length.out = 101)
stopifnot(isTRUE(all.equal(A$exp(x), B$exp(x))))

# 2. Forcing a target in one package does not affect the other.
usable_a <- intersect(ta$compiled, ta$supported)
usable_b <- intersect(tb$compiled, tb$supported)
if (length(usable_a) >= 2L) {
  stopifnot(A$force(usable_a[2]))
  stopifnot(identical(A$targets()$dispatched, usable_a[2]))
  stopifnot(identical(B$targets()$dispatched, usable_b[1]))
  stopifnot(A$force(""))
  cat("target forcing isolated (A forced to", usable_a[2], ")\n")
} else {
  cat("only one usable target here; target-forcing check skipped\n")
}

# 3. Abort handlers are separate.
A$handler(TRUE)
a0 <- A$calls(); b0 <- B$calls()
eb <- tryCatch({ B$abort(4L); NULL }, error = conditionMessage)
stopifnot(grepl("consumerB kernel abort (code 4)", eb, fixed = TRUE))
stopifnot(A$calls() == a0, B$calls() == b0)
ea <- tryCatch({ A$abort(6L); NULL }, error = conditionMessage)
stopifnot(grepl("consumerA kernel abort (code 6)", ea, fixed = TRUE))
stopifnot(A$calls() == a0 + 1L, B$calls() == b0)
A$handler(FALSE)

cat("isolation test passed:", mode, "\n")
