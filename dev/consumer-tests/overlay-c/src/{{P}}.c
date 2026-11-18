/* .Call glue written in C. The SIMD kernels are C++ (highway-kernels.cpp,
 * created by use_highway(language = "c")) behind a plain C interface. */
#include <R.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>
#include <Rinternals.h>
#include <stddef.h>

#include "highway-kernels.h"

int {{P}}_hwy_abort(int code, char* err, size_t err_len);

/* A stand-in for another library that takes a C callback. */
typedef int (*kernel_fn)(const double*, double*, size_t, char*, size_t);
static int apply_callback(kernel_fn fn, const double* x, double* out,
                          size_t n, char* err, size_t err_len) {
  return fn(x, out, n, err, err_len);
}

static SEXP checked(int status, const char* err, SEXP value) {
  if (status != HIGHWAY_STATUS_OK) Rf_error("kernel failed (%d): %s", status, err);
  return value;
}

SEXP c_add(SEXP x, SEXP y) {
  R_xlen_t n = XLENGTH(x);
  if (XLENGTH(y) != n) Rf_error("lengths differ");
  SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
  char err[256];
  int status = {{P}}_hwy_add(REAL(x), REAL(y), REAL(out), (size_t) n, err, sizeof err);
  UNPROTECT(1);
  return checked(status, err, out);
}

SEXP c_exp(SEXP x) {
  R_xlen_t n = XLENGTH(x);
  SEXP out = PROTECT(Rf_allocVector(REALSXP, n));
  char err[256];
  /* Called through a C function pointer, as a host library would. */
  int status = apply_callback({{P}}_hwy_exp, REAL(x), REAL(out), (size_t) n,
                              err, sizeof err);
  UNPROTECT(1);
  return checked(status, err, out);
}

/* Returns list(status, message) instead of raising, so tests can inspect it. */
SEXP c_abort(SEXP code) {
  char err[256];
  int status = {{P}}_hwy_abort(Rf_asInteger(code), err, sizeof err);
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 2));
  SET_VECTOR_ELT(out, 0, Rf_ScalarInteger(status));
  SET_VECTOR_ELT(out, 1, Rf_mkString(err));
  UNPROTECT(1);
  return out;
}

SEXP c_null_args(void) {
  char err[256];
  int status = {{P}}_hwy_add(NULL, NULL, NULL, 10, err, sizeof err);
  return Rf_ScalarInteger(status);
}

#define ENTRY(name, n) {#name, (DL_FUNC) (void (*)(void)) &name, n}
static const R_CallMethodDef call_entries[] = {
    ENTRY(c_add, 2), ENTRY(c_exp, 1), ENTRY(c_abort, 1), ENTRY(c_null_args, 0),
    {NULL, NULL, 0}};

void attribute_visible R_init_{{P}}(DllInfo *dll) {
  R_registerRoutines(dll, NULL, call_entries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
