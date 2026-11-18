#ifndef {{P}}_EXTRA_H_
#define {{P}}_EXTRA_H_

#include <stddef.h>
#include <stdint.h>

namespace {{P}}_extra {
int64_t dispatched_target();
int64_t compiled_targets();
void abort_kernel(int code);
void log_doubles(const double* in, double* out, size_t n);
}  // namespace {{P}}_extra

#endif  // {{P}}_EXTRA_H_
