# This file is distributed under the BSD 3-Clause License. See LICENSE for details.

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

cc_library(
    name = "batage",
    srcs = glob(
        ["*.cpp"],
        exclude = ["*_test*.cpp", "*_bench*.cpp", "emulator.cpp"],
    ),
    hdrs = glob(["*.hpp"]),
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "//gold:gold",
        "@dromajo",
    ]
)

