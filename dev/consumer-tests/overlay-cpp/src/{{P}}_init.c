/* Routine registration for consumer package {{P}}, in C: R compiles and links
 * this together with the package's C++ files without extra settings. */
#include <R.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>
#include <Rinternals.h>

extern SEXP {{P}}_add(SEXP, SEXP);
extern SEXP {{P}}_exp(SEXP);
extern SEXP {{P}}_log(SEXP);
extern SEXP {{P}}_targets(void);
extern SEXP {{P}}_force_target(SEXP);
extern SEXP {{P}}_chosen_initialized(void);
extern SEXP {{P}}_deinit(void);
extern SEXP {{P}}_warm_up(void);
extern SEXP {{P}}_set_abort_handler(SEXP);
extern SEXP {{P}}_handler_calls(void);
extern SEXP {{P}}_abort(SEXP);
extern SEXP {{P}}_abort_workers(SEXP, SEXP);
extern SEXP {{P}}_exp_threads(SEXP, SEXP, SEXP);

#define ENTRY(name, n) {#name, (DL_FUNC) (void (*)(void)) &name, n}

static const R_CallMethodDef call_entries[] = {
    ENTRY({{P}}_add, 2),
    ENTRY({{P}}_exp, 1),
    ENTRY({{P}}_log, 1),
    ENTRY({{P}}_targets, 0),
    ENTRY({{P}}_force_target, 1),
    ENTRY({{P}}_chosen_initialized, 0),
    ENTRY({{P}}_deinit, 0),
    ENTRY({{P}}_warm_up, 0),
    ENTRY({{P}}_set_abort_handler, 1),
    ENTRY({{P}}_handler_calls, 0),
    ENTRY({{P}}_abort, 1),
    ENTRY({{P}}_abort_workers, 2),
    ENTRY({{P}}_exp_threads, 3),
    {NULL, NULL, 0}};

void attribute_visible R_init_{{P}}(DllInfo *dll) {
  R_registerRoutines(dll, NULL, call_entries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
