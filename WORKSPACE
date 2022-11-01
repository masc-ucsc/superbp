load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# google benchmark
http_archive(
    name = "com_google_benchmark",
    sha256 = "ef0fab8058be682e382e155eeda1b22e1747fd6606e22e0c1b19f6da91e8b52d",
    strip_prefix = "benchmark-69054ae50e07e9de7cb27f9e2d1d355f74605524",
    urls = ["https://github.com/google/benchmark/archive/69054ae50e07e9de7cb27f9e2d1d355f74605524.zip"],
)

# google tests
http_archive(
  name = "com_google_googletest",
  urls = ["https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip"],
  strip_prefix = "googletest-609281088cfefc76f9d0ce82e1ff6c30cc3591e5",
)

# git_repository(
#     name = "glog",
#     remote = "https://github.com/google/glog.git",
#     tag = "v0.4.0",
# )

# abseil
http_archive(
  name = "com_google_absl",
  strip_prefix = "abseil-cpp-20211102.0",
  urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20211102.0.zip"],
  sha256 = "a4567ff02faca671b95e31d315bab18b42b6c6f1a60e91c6ea84e5a2142112c2",
)

# git_repository(
#     name = "com_github_gflags_gflags",
#     remote = "https://github.com/gflags/gflags.git",
#     tag = "v2.2.2",
# )

# fmt
http_archive(
    name = "fmt",
    build_file = "BUILD.fmt",
    sha256 = "fccfc86e4aff6c33bff9a1d907b2ba2de2a5a8ab84349be4964a93f8d8c21b62",
    strip_prefix = "fmt-7bdf0628b1276379886c7f6dda2cef2b3b374f0b",
    urls = [
        "https://github.com/fmtlib/fmt/archive/7bdf0628b1276379886c7f6dda2cef2b3b374f0b.zip",
    ],
)

# Dromajo
http_archive(
    name = "dromajo",
    build_file = "dromajo.BUILD",
    patches = ["//external:dromajo.patch"],
    sha256 = "5b10b48c4b0db08108ebb95e425326589c72c813437644113e3137570a4b7280",
    strip_prefix = "dromajo-4b97add1024060b1e9d335a5d76af85276dbad02",
    urls = [
        "https://github.com/masc-ucsc/dromajo/archive/4b97add1024060b1e9d335a5d76af85276dbad02.zip",
    ],
)

# libelf (needed by dromajo in OSX)
http_archive(
    name = "libelf",
    build_file = "libelf.BUILD",
    sha256 = "c0627b45c29a151e4e1105988ad7ce9bf83b52cbbca0a1db06c7fcad69b85c4b",
    strip_prefix = "libelf-ba3c81450b91d1935fff01bae191a59d7653d2a5",
    urls = [
        "https://github.com/masc-ucsc/libelf/archive/ba3c81450b91d1935fff01bae191a59d7653d2a5.zip",
    ],
)

