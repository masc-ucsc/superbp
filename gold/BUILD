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
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "gold_bench",
    srcs = ["gold_bench.cpp"],
    deps = [
      ":gold",
      "//external:dromajo_masc",
        "@com_google_benchmark//:benchmark",
    ],
)
