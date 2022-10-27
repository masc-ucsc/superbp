load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "com_google_googletest",
    remote = "https://github.com/google/googletest",
    tag = "release-1.11.0",
)

git_repository(
    name = "glog",
    remote = "https://github.com/google/glog.git",
    tag = "v0.4.0",
)

git_repository(
    name = "com_google_absl",
    remote = "https://github.com/abseil/abseil-cpp.git",
    tag = "20200225.2",
)

git_repository(
    name = "com_google_benchmark",
    remote = "https://github.com/google/benchmark.git",
    tag = "v1.5.1",
)

git_repository(
    name = "com_github_gflags_gflags",
    remote = "https://github.com/gflags/gflags.git",
    tag = "v2.2.2",
)

http_archive(
    name = "dromajo",
    build_file = "BUILD.dromajo",
    patches = ["//external:dromajo.patch"],
    sha256 = "03aa13c7479b74804eae15a51d2df66b4cd49eb2cdb3a0119521268fc7160167",
    strip_prefix = "dromajo-753f4c43bda3dfd05becadd81bd0041cec240910",
    urls = [
        "https://github.com/masc-ucsc/dromajo/archive/753f4c43bda3dfd05becadd81bd0041cec240910.zip",
    ],
)

http_archive(
    name = "fmt",
    build_file = "BUILD.fmt",
    sha256 = "fccfc86e4aff6c33bff9a1d907b2ba2de2a5a8ab84349be4964a93f8d8c21b62",
    strip_prefix = "fmt-7bdf0628b1276379886c7f6dda2cef2b3b374f0b",
    urls = [
        "https://github.com/fmtlib/fmt/archive/7bdf0628b1276379886c7f6dda2cef2b3b374f0b.zip",
    ],
)
