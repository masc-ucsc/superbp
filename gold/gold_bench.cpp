//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "benchmark/benchmark.h"

#include "imli.hpp"

#include <chrono>
#include <string>

void code_to_benchmark() {
  std::string s1(1024, '-');
  std::string s2(1024, '-');
  benchmark::DoNotOptimize(s1.compare(s2));
}

static void BM_prediction_speed(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    code_to_benchmark();
  }
}

BENCHMARK(BM_prediction_speed);

// Run the benchmark
BENCHMARK_MAIN();