// tests/test_get_mime_type.cpp

#include <gtest/gtest.h>
#include "../utils.hpp"

TEST(GetMimeTypeTest, KnownExtensions) {
    EXPECT_EQ(get_mime_type("index.html"), "text/html");
    EXPECT_EQ(get_mime_type("style.css"), "text/css");
    EXPECT_EQ(get_mime_type("script.js"), "application/javascript");
    EXPECT_EQ(get_mime_type("data.json"), "application/json");
    EXPECT_EQ(get_mime_type("image.png"), "image/png");
    EXPECT_EQ(get_mime_type("photo.jpg"), "image/jpeg");
    EXPECT_EQ(get_mime_type("graphic.gif"), "image/gif");
}

TEST(GetMimeTypeTest, UnknownExtensions) {
    EXPECT_EQ(get_mime_type("archive.zip"), "application/octet-stream");
    EXPECT_EQ(get_mime_type("video.mp4"), "application/octet-stream");
    EXPECT_EQ(get_mime_type("audio.mp3"), "application/octet-stream");
    EXPECT_EQ(get_mime_type("README"), "application/octet-stream");
}
