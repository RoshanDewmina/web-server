// tests/test_parse_request.cpp

#include <gtest/gtest.h>
#include "../server.cpp" // Or better, refactor parse_request into a separate header

TEST(ParseRequestTest, GETRequest) {
    std::string request =
        "GET /about HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n";

    auto request_info = parse_request(request);

    EXPECT_EQ(request_info["method"], "GET");
    EXPECT_EQ(request_info["path"], "/about");
    EXPECT_EQ(request_info["version"], "HTTP/1.1");
    EXPECT_EQ(request_info["Host"], "localhost:8080");
    EXPECT_EQ(request_info["Connection"], "close");
    EXPECT_EQ(request_info["body"], "");
}

TEST(ParseRequestTest, POSTRequestWithBody) {
    std::string request =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 13\r\n"
        "Connection: close\r\n"
        "\r\n"
        "name=JohnDoe";

    auto request_info = parse_request(request);

    EXPECT_EQ(request_info["method"], "POST");
    EXPECT_EQ(request_info["path"], "/submit");
    EXPECT_EQ(request_info["version"], "HTTP/1.1");
    EXPECT_EQ(request_info["Host"], "localhost:8080");
    EXPECT_EQ(request_info["Content-Type"], "application/x-www-form-urlencoded");
    EXPECT_EQ(request_info["Content-Length"], "13");
    EXPECT_EQ(request_info["Connection"], "close");
    EXPECT_EQ(request_info["body"], "name=JohnDoe");
}

TEST(ParseRequestTest, MalformedRequest) {
    std::string request = "BADREQUEST";

    auto request_info = parse_request(request);

    EXPECT_EQ(request_info["method"], "BADREQUEST");
    EXPECT_EQ(request_info["path"], "");
    EXPECT_EQ(request_info["version"], "");
    EXPECT_EQ(request_info["body"], "");
}
