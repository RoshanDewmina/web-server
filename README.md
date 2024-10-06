# C++ Multi-threaded Web Server with Docker Support

This project is a multi-threaded web server written in C++, featuring SSL/TLS support, routing, configuration files, and Docker integration. It serves static files, handles GET and POST requests, and can be easily extended with additional features.

---

## Table of Contents

- [Features](#features)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
  - [Clone the Repository](#1-clone-the-repository)
  - [Generate SSL Certificates](#2-generate-ssl-certificates)
  - [Build the Docker Image](#3-build-the-docker-image)
- [Usage](#usage)
  - [Running with Docker](#running-with-docker)
  - [Running with Docker Compose](#running-with-docker-compose)
- [Configuration](#configuration)
  - [Using `config.json`](#using-configjson)
  - [Using Environment Variables](#using-environment-variables)
- [Generating SSL Certificates](#generating-ssl-certificates)
- [Testing the Server](#testing-the-server)
- [Project Structure](#project-structure)
- [Docker Details](#docker-details)
  - [Dockerfile Explanation](#dockerfile-explanation)
  - [Multi-Stage Builds](#multi-stage-builds)
- [Contributing](#contributing)
- [License](#license)

---

## Features

- **Multi-threaded Server**: Handles multiple client connections simultaneously using a thread pool.
- **SSL/TLS Support**: Secure communication using OpenSSL.
- **Routing Mechanism**: Maps URLs to handler functions for clean and flexible request handling.
- **Configuration File**: Reads server parameters from `config.json`.
- **Logging**: Writes logs to a file with timestamps.
- **POST and GET Method Support**: Handles both GET and POST HTTP methods.
- **Docker Integration**: Dockerfile and Docker Compose support for containerization.
- **Serve Static Files**: Serves files from a designated web root directory.

---

## Prerequisites

- **Docker**: Install Docker from [here](https://www.docker.com/products/docker-desktop).
- **OpenSSL**: Required for generating SSL certificates.
- **Git**: For cloning the repository.
- **C++17 Compiler**: If you plan to build and run the server without Docker.

---

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/your_username/your_repository.git
cd your_repository
```

### 2. Generate SSL Certificates

Generate self-signed SSL certificates required for HTTPS.

```bash
openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.crt
```

- **Note**: Keep the `server.key` and `server.crt` files in the project root directory. **Do not share these files publicly.**

### 3. Build the Docker Image

```bash
docker build -t my_cpp_server .
```

---

## Usage

### Running with Docker

```bash
docker run -d -p 8080:8080 --name cpp_server_container my_cpp_server
```

- **`-d`**: Runs the container in detached mode.
- **`-p 8080:8080`**: Maps port 8080 on the host to port 8080 in the container.
- **`--name cpp_server_container`**: Names the container for easy reference.

### Running with Docker Compose

If you prefer using Docker Compose:

1. **Modify `docker-compose.yml`** (if necessary):

   ```yaml
   version: '3'
   services:
     web:
       build: .
       ports:
         - "8080:8080"
       volumes:
         - ./www:/usr/src/app/www
       environment:
         - PORT=8080
         - MAX_THREADS=4
   ```

2. **Run with Docker Compose**:

   ```bash
   docker-compose up -d
   ```

---

## Configuration

### Using `config.json`

The server reads configuration parameters from `config.json`. Create this file in the project root directory if it doesn't exist.

**Sample `config.json`**:

```json
{
    "port": 8080,
    "max_threads": 4,
    "web_root": "./www"
}
```

- **`port`**: The port number the server listens on.
- **`max_threads`**: Maximum number of threads in the thread pool.
- **`web_root`**: The directory where static files are served from.

### Using Environment Variables

Environment variables can override values in `config.json`.

- **`PORT`**: Overrides the `port` value.
- **`MAX_THREADS`**: Overrides the `max_threads` value.
- **`WEB_ROOT`**: Overrides the `web_root` value.

**Example**:

```bash
docker run -d -p 8081:8081 \
  -e PORT=8081 \
  -e MAX_THREADS=8 \
  --name cpp_server_container \
  my_cpp_server
```

---

## Generating SSL Certificates

To enable HTTPS, generate self-signed SSL certificates:

```bash
openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.crt
```

- **Keep these files secure** and do not commit them to version control.
- **For production**, obtain certificates from a trusted Certificate Authority (CA).

---

## Testing the Server

### Using a Web Browser

- Navigate to `https://localhost:8080/`.
- Proceed past any security warnings due to the self-signed certificate.

### Using `curl`

```bash
curl -k https://localhost:8080/
```

- **`-k`**: Allows `curl` to proceed with insecure connections (self-signed certificates).

### Testing POST Requests

```bash
curl -k -X POST -d "name=John&age=30" https://localhost:8080/
```

---

## Project Structure

```
your_repository/
├── Dockerfile
├── docker-compose.yml
├── server.cpp
├── json.hpp
├── config.sample.json
├── www/
│   ├── index.html
│   └── about.html
├── .gitignore
├── README.md
└── .dockerignore
```

- **`server.cpp`**: Main server source code.
- **`json.hpp`**: JSON parsing library header.
- **`config.sample.json`**: Sample configuration file.
- **`www/`**: Directory containing static files.
- **`.gitignore`**: Specifies intentionally untracked files to ignore.
- **`Dockerfile`**: Instructions to build the Docker image.
- **`docker-compose.yml`**: Configuration for Docker Compose.
- **`README.md`**: Project documentation.
- **`.dockerignore`**: Files and directories to ignore during Docker build.

---

## Docker Details

### Dockerfile Explanation

The Dockerfile uses multi-stage builds to create a lightweight image.

```dockerfile
# Build Stage
FROM gcc:11.2.0 AS builder
RUN apt-get update && apt-get install -y libssl-dev
WORKDIR /usr/src/app
COPY . .
RUN g++ -std=c++17 -pthread server.cpp -o server -lssl -lcrypto

# Runtime Stage
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y libssl-dev
WORKDIR /usr/src/app
COPY --from=builder /usr/src/app/server .
COPY config.json .
COPY server.crt .
COPY server.key .
COPY www ./www
EXPOSE 8080
CMD ["./server"]
```

- **Builder Stage**: Compiles the server.
- **Runtime Stage**: Runs the server with only necessary dependencies.

### Multi-Stage Builds

- **Benefit**: Reduces the final image size by excluding build tools and intermediate files.
- **Security**: Minimizes the attack surface by including only what is necessary.

---

## Contributing

Contributions are welcome! Please follow these steps:

1. **Fork the Repository**: Click on the 'Fork' button on GitHub.
2. **Clone Your Fork**:

   ```bash
   git clone https://github.com/your_username/your_repository.git
   ```

3. **Create a Branch**:

   ```bash
   git checkout -b feature/your_feature
   ```

4. **Make Changes**: Implement your feature or fix.
5. **Commit Changes**:

   ```bash
   git commit -am "Add your message here"
   ```

6. **Push to Your Fork**:

   ```bash
   git push origin feature/your_feature
   ```

7. **Submit a Pull Request**: Go to the original repository and open a pull request.

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

**Disclaimer**: This server is intended for educational purposes. In a production environment, ensure proper security measures are in place, including using certificates from trusted Certificate Authorities and thoroughly testing the application.

---

**Contact**: For any questions or suggestions, feel free to open an issue or contact the repository owner.

---

Happy Coding!