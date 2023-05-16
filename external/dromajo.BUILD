# This file is distributed under the BSD 3-Clause License. See LICENSE for details.

load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "dromajo",
    srcs = glob(
        ["src/*.cpp"],
        exclude = ["src/*dromajo_cosim.cpp", "src/*_test.cpp", "src/dromajo.cpp"],
    ),
    hdrs = glob(["include/*.h"]),
    includes = ["include"],
    defines = ["SIMPOINT_BB=1", "SIMPOINT_BBhartid=1"],
    visibility = ["//visibility:public"],
    deps = [
      "@libelf"
    ],
)

