load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

BAZEL_CXXOPTS="-std=c++11"

cc_library(
    name = "dromajo_masc",
    srcs = glob(
        ["*.cpp"],
        exclude = ["*test*.cpp", "*bench*.cpp"],
    ),
    hdrs = glob(["*.hpp"]),
    visibility = ["//visibility:public"],
)

