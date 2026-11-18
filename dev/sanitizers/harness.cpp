// Standalone sanitizer harness for the R package 'highway' (no R needed).
// Links the package's demo kernels (src/demo_kernels.cpp) with Highway's
// runtime (src/highway-impl.cpp, i.e. highway/impl.h) and exercises them from
// many threads: concurrent first dispatch after resetting the chosen target,
// and HWY_ABORT on worker threads caught by highway::c_guard. Built and run
// under ThreadSanitizer and AddressSanitizer+UBSan by dev/sanitizers/run.sh.
#include <stdio.h>
#include <string.h>

#include <atomic>
#include <thread>
#include <vector>

#include "demo_kernels.h"
#include "highway/c_guard.h"
#include "highway/runtime.h"
#include "hwy/targets.h"

int main() {
  const size_t n = 100003;
  const int nthreads = 8;
  std::vector<double> x(n);
  for (size_t i = 0; i < n; ++i) x[i] = -50.0 + 100.0 * i / (n - 1);
  std::vector<double> ref(n);
  highway_demo::Exp(x.data(), ref.data(), n);
  const double ref_sum = highway_demo::Sum(x.data(), n, false);

  int failures = 0;
  for (int rep = 0; rep < 50; ++rep) {
    hwy::GetChosenTarget().DeInit();  // force a cold, concurrent first call
    std::vector<std::vector<double>> out(nthreads, std::vector<double>(n));
    std::vector<double> sums(nthreads);
    std::atomic<int> ready{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < nthreads; ++t) {
      pool.emplace_back([&, t] {
        ready.fetch_add(1);
        while (ready.load() < nthreads) {
        }
        highway_demo::Exp(x.data(), out[t].data(), n);
        sums[t] = highway_demo::Sum(x.data(), n, false);
        (void)highway_demo::DispatchedTarget();
      });
    }
    for (auto& th : pool) th.join();
    for (int t = 0; t < nthreads; ++t) {
      if (memcmp(out[t].data(), ref.data(), n * sizeof(double)) != 0 ||
          sums[t] != ref_sum) {
        ++failures;
      }
    }
  }

  std::vector<int> status(nthreads, -1);
  std::vector<std::thread> pool;
  for (int t = 0; t < nthreads; ++t) {
    pool.emplace_back([&, t] {
      char msg[128];
      status[t] = highway::c_guard([] { highway_demo::AbortInKernel(5); }, msg,
                                   sizeof(msg));
      if (strstr(msg, "abort requested (code 5)") == nullptr) status[t] = -2;
    });
  }
  for (auto& th : pool) th.join();
  for (int s : status) {
    if (s != HIGHWAY_STATUS_ABORT) ++failures;
  }

  printf("dispatched target: %s\n",
         hwy::TargetName(highway_demo::DispatchedTarget()));
  printf("supported:");
  highway::for_each_target(hwy::SupportedTargets(),
                           [](int64_t, const char* nm) { printf(" %s", nm); });
  printf("\n%s (%d failures)\n", failures ? "FAILED" : "passed", failures);
  return failures ? 1 : 0;
}
