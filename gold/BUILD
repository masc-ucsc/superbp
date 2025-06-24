load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

cc_library(
  name = "headers",
  hdrs = glob(["*.hpp"]),
)
cc_test(
  name = "gold_test",
  srcs = ["gold_test.cpp"],
  deps = [
    ":headers",
    "@fmt//:fmt",
    "//batage:batage",
    "@googletest//:gtest_main",
  ],
)

cc_binary(
  name = "gold_bench",
  srcs = ["gold_bench.cpp"],
  deps = [
    ":headers",
    "@fmt//:fmt",
    # "@dromajo",
    "@google_benchmark//:benchmark",
  ],
)
