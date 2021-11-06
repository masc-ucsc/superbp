
//#include "imli.hpp"

#include <string>
#include "fmt/format.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

class Gold_test : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {
    // Graph_library::sync_all();
  }
};

TEST_F(Gold_test, Trivial_imli_test1) {

  // instantiate IMLI, some configuration and test the basic API so that it it
  // learns. It is also a way to showcase the API

  fmt::print("hello world\n");

  EXPECT_NE(10,100);
}

