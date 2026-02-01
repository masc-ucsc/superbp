# This file is distributed under the BSD 3-Clause License. See LICENSE for details.

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
  name = "headers",
  hdrs = [
    "emulator.hpp",
  ],
)

cc_binary(
  name = "emulator",
  srcs = [
    "emulator.cpp",
  ],
  deps = [
    ":headers",
    "//batage:batage",
    "@dromajo",
  ],
)
