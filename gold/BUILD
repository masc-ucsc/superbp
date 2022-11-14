load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

cc_library(
    name = "gold",
    srcs = glob(
        ["*.cpp"],
        exclude = ["*test*.cpp", "*bench*.cpp"],
    ),
    hdrs = glob(["*.hpp"]),
    visibility = ["//visibility:public"],
    deps = [
      "@fmt",
    ],
)

cc_test(
    name = "gold_test",
    srcs = ["gold_test.cpp"],
    deps = [
      ":gold",
      "//batage:batage",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "gold_bench",
    srcs = ["gold_bench.cpp"],
    deps = [
      ":gold",
      "@dromajo",
      "@com_google_benchmark//:benchmark",
    ],
)
