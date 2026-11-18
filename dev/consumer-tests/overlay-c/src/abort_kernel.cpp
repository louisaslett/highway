// An extern "C" entry point that aborts inside a dispatched Highway kernel,
// guarded by highway::c_guard: the C side only ever sees a status code.
#include <stddef.h>

#include "highway/c_guard.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "abort_kernel.cpp"
#include "hwy/foreach_target.h"  // IWYU pragma: keep

#include "hwy/highway.h"

HWY_BEFORE_NAMESPACE();
namespace {{P}}_abort {
namespace HWY_NAMESPACE {
namespace {
void AbortImpl(int code) {
  if (code != 0) HWY_ABORT("{{P}} C-interface abort (code %d)", code);
}
}  // namespace
// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace {{P}}_abort
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace {{P}}_abort {
HWY_EXPORT(AbortImpl);
}  // namespace {{P}}_abort

extern "C" int {{P}}_hwy_abort(int code, char* err, size_t err_len) {
  return highway::c_guard(
      [&] {
        using namespace {{P}}_abort;
        HWY_DYNAMIC_DISPATCH(AbortImpl)(code);
      },
      err, err_len);
}
#endif  // HWY_ONCE
