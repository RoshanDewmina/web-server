// tests/test_ends_with.cpp

#include <gtest/gtest.h>
#include "../utils.hpp"

// Test suite for ends_with
TEST(EndsWithTest, BasicAssertions) {
    EXPECT_TRUE(ends_with("filename.html", ".html"));
    EXPECT_TRUE(ends_with("style.css", ".css"));
    EXPECT_TRUE(ends_with("script.js", ".js"));
    EXPECT_TRUE(ends_with("image.jpeg", ".jpeg"));
    EXPECT_FALSE(ends_with("document.txt", ".html"));
    EXPECT_FALSE(ends_with("archive.tar.gz", ".zip"));
    EXPECT_TRUE(ends_with(".hiddenfile", ".hiddenfile"));
    EXPECT_FALSE(ends_with("short", "longer_suffix"));
}
