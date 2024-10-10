// server.cpp

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <streambuf>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <filesystem>
#include <ctime>


#include "utils.hpp"


// OpenSSL Headers
#include <openssl/ssl.h>
#include <openssl/err.h>



// JSON Parsing Library
#include "json.hpp"
using json = nlohmann::json;

namespace fs = std::filesystem;

// Configuration Variables
int PORT;
int MAX_THREADS;
std::string WEB_ROOT;

// Forward declarations
void handle_client(int client_socket, SSL_CTX *ctx);

// Thread Pool Class Definition
class ThreadPool {
public:
    ThreadPool(size_t num_threads, SSL_CTX *ctx);
    ~ThreadPool();
    void enqueue(int client_socket);

private:
    void worker();
    std::vector<std::thread> workers;
    std::queue<int> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
    SSL_CTX *ctx;
};

// Thread Pool Class Implementation
ThreadPool::ThreadPool(size_t num_threads, SSL_CTX *ctx) : ctx(ctx) {
    for (size_t i = 0; i < num_threads; ++i)
        workers.emplace_back([this] { worker(); });
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers)
        worker.join();
}

void ThreadPool::enqueue(int client_socket) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(client_socket);
    }
    condition.notify_one();
}

void ThreadPool::worker() {
    while (true) {
        int client_socket;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty())
                return;
            client_socket = tasks.front();
            tasks.pop();
        }
        handle_client(client_socket, ctx);
    }
}

// Global mutex for logging
std::mutex log_mutex;
std::ofstream log_file;

// Function to log messages
void log(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    log_file << "[" << buf << "] " << message << std::endl;
}

// Function to read the entire request from the client (SSL version)
std::string ssl_read_request(SSL *ssl) {
    std::string request;
    char buffer[2048];
    int bytes_read;

    // Read the request line and headers
    while ((bytes_read = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
        request.append(buffer, bytes_read);
        if (request.find("\r\n\r\n") != std::string::npos) {
            break; // End of headers
        }
    }

    // Parse headers to find Content-Length
    size_t content_length = 0;
    size_t pos = request.find("Content-Length:");
    if (pos != std::string::npos) {
        size_t end_of_line = request.find("\r\n", pos);
        std::string content_length_str = request.substr(pos + 15, end_of_line - pos - 15);
        content_length = std::stoul(content_length_str);
    }

    // Read the body if Content-Length is specified
    if (content_length > 0) {
        size_t total_bytes_read = 0;
        while (total_bytes_read < content_length) {
            bytes_read = SSL_read(ssl, buffer, std::min(sizeof(buffer), content_length - total_bytes_read));
            if (bytes_read <= 0) break;
            request.append(buffer, bytes_read);
            total_bytes_read += bytes_read;
        }
    }

    return request;
}

// Function to parse the HTTP request
std::unordered_map<std::string, std::string> parse_request(const std::string& request) {
    std::unordered_map<std::string, std::string> request_info;
    std::istringstream request_stream(request);
    std::string line;

    // Get the request line
    if (std::getline(request_stream, line)) {
        std::istringstream line_stream(line);
        line_stream >> request_info["method"];
        line_stream >> request_info["path"];
        line_stream >> request_info["version"];
    }

    // Parse headers
    while (std::getline(request_stream, line) && line != "\r") {
        if (line == "\r" || line.empty()) {
            break;
        }
        auto colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string header_name = line.substr(0, colon_pos);
            std::string header_value = line.substr(colon_pos + 1);
            // Trim whitespace
            header_value.erase(0, header_value.find_first_not_of(" \t"));
            header_value.erase(header_value.find_last_not_of(" \t\r\n") + 1);
            request_info[header_name] = header_value;
        }
    }

    // Read the body
    std::string body;
    std::getline(request_stream, body, '\0'); // Read until EOF
    request_info["body"] = body;

    return request_info;
}

// Function to get the MIME type based on file extension
std::string get_mime_type(const std::string& path) {
    if (path.ends_with(".html") || path.ends_with(".htm"))
        return "text/html";
    else if (path.ends_with(".css"))
        return "text/css";
    else if (path.ends_with(".js"))
        return "application/javascript";
    else if (path.ends_with(".json"))
        return "application/json";
    else if (path.ends_with(".png"))
        return "image/png";
    else if (path.ends_with(".jpg") || path.ends_with(".jpeg"))
        return "image/jpeg";
    else if (path.ends_with(".gif"))
        return "image/gif";
    else
        return "application/octet-stream";
}

// Function to handle 404 Not Found
std::string not_found_response() {
    std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
    std::string response;
    response = "HTTP/1.1 404 Not Found\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    return response;
}

// Function to handle root path
std::string handle_root(const std::unordered_map<std::string, std::string>& request_info) {
    // Serve index.html
    std::string full_path = WEB_ROOT + "/index.html";
    std::string body;
    std::string response;

    if (fs::exists(full_path) && !fs::is_directory(full_path)) {
        // Serve the file
        std::ifstream file(full_path, std::ios::binary);
        if (file) {
            body.assign((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
            std::string content_type = get_mime_type(full_path);

            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
            response += "Content-Type: " + content_type + "\r\n";
            response += "Connection: close\r\n";
            response += "\r\n";
            response += body;
        } else {
            response = not_found_response();
        }
    } else {
        response = not_found_response();
    }

    return response;
}

// Function to handle /about path
std::string handle_about(const std::unordered_map<std::string, std::string>& request_info) {
    std::string body = "<html><body><h1>About Us</h1><p>This is the about page.</p></body></html>";
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    return response;
}

// Function to handle POST requests
std::string handle_post(const std::unordered_map<std::string, std::string>& request_info) {
    // For demonstration, echo back the received data
    std::string received_data = request_info.at("body");
    std::string body = "<html><body><h1>POST Data Received</h1><pre>" + received_data + "</pre></body></html>";
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    return response;
}

// Define a Handler Function Type
using HandlerFunc = std::string(*)(const std::unordered_map<std::string, std::string>&);

// Routing Table
std::unordered_map<std::string, HandlerFunc> routes;

// Initialize Routes
void initialize_routes() {
    routes["/"] = handle_root;
    routes["/about"] = handle_about;
    // Add more routes as needed
}

// Function to generate the HTTP response
std::string generate_response(const std::unordered_map<std::string, std::string>& request_info) {
    std::string method = request_info.at("method");
    std::string path = request_info.at("path");

    // Handle POST requests
    if (method == "POST") {
        return handle_post(request_info);
    }

    // Check if the path is in the routing table
    if (routes.find(path) != routes.end()) {
        return routes[path](request_info);
    } else {
        // Serve static files or return 404
        std::string full_path = WEB_ROOT + path;
        std::string response;
        std::string body;
        std::string content_type;

        if (fs::exists(full_path) && !fs::is_directory(full_path)) {
            // Serve the file
            std::ifstream file(full_path, std::ios::binary);
            if (file) {
                body.assign((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
                content_type = get_mime_type(full_path);

                response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
                response += "Content-Type: " + content_type + "\r\n";
                response += "Connection: close\r\n";
                response += "\r\n";
                response += body;
            } else {
                response = not_found_response();
            }
        } else {
            response = not_found_response();
        }

        return response;
    }
}

// Function to handle each client connection
void handle_client(int client_socket, SSL_CTX *ctx) {
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_socket);

    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        // Read the request using SSL_read
        std::string request = ssl_read_request(ssl);

        if (request.empty()) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_socket);
            return;
        }

        // Parse the request
        auto request_info = parse_request(request);

        // Generate the response
        std::string response = generate_response(request_info);

        // Send the response using SSL_write
        ssize_t bytes_sent = SSL_write(ssl, response.c_str(), response.size());
        if (bytes_sent <= 0) {
            log("Failed to send response to client.");
        }

        // Log the request
        log("[" + request_info["method"] + "] " + request_info["path"] + " - Thread: " +
            std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client_socket);
}

int main() {
    // Open log file
    log_file.open("server.log", std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
        return -1;
    }

    // Read configuration file
    std::ifstream config_file("config.json");
    if (!config_file.is_open()) {
        log("Failed to open config.json");
        return -1;
    }

    json config;
    config_file >> config;

    PORT = config.value("port", 8080);
    MAX_THREADS = config.value("max_threads", 4);
    WEB_ROOT = config.value("web_root", "./www");

    // At the beginning of main(), after variable declarations
char* port_env = std::getenv("PORT");
if (port_env != nullptr) {
    PORT = std::stoi(port_env);
}

char* max_threads_env = std::getenv("MAX_THREADS");
if (max_threads_env != nullptr) {
    MAX_THREADS = std::stoi(max_threads_env);
}

char* web_root_env = std::getenv("WEB_ROOT");
if (web_root_env != nullptr) {
    WEB_ROOT = web_root_env;
}


    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        log("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Load certificates
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Initialize routes
    initialize_routes();

    int server_socket;
    sockaddr_in server_addr{};

    // Create the server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        log("Failed to create socket.");
        return -1;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log("setsockopt failed.");
        close(server_socket);
        return -1;
    }

    // Configure the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log("Failed to bind socket.");
        close(server_socket);
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) < 0) {
        log("Failed to listen on socket.");
        close(server_socket);
        return -1;
    }

    log("Server is listening on port " + std::to_string(PORT));

    // Create a thread pool
    ThreadPool pool(MAX_THREADS, ctx);

    while (true) {
        // Accept a new client connection
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            log("Failed to accept client connection.");
            continue;
        }

        // Enqueue the client socket to the thread pool
        pool.enqueue(client_socket);
    }

    // Close the server socket (unreachable code in this example)
    close(server_socket);

    // Close log file
    log_file.close();

    // Clean up OpenSSL
    SSL_CTX_free(ctx);
    EVP_cleanup();

    return 0;
}
