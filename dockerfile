# Build Stage
FROM gcc:11.2.0 AS builder

# Install necessary packages
RUN apt-get update && apt-get install -y \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy the current directory contents into the container
COPY . .

# Build the application
RUN g++ -std=c++20 -pthread server.cpp -o server \
    -lssl -lcrypto

# Runtime Stage
FROM ubuntu:20.04

# Install necessary packages for runtime
RUN apt-get update && apt-get install -y \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy the compiled application from the builder stage
COPY --from=builder /usr/src/app/server .

# Copy necessary files
COPY config.json .
COPY server.crt .
COPY server.key .
COPY www ./www

# Expose the port the server listens on (match with your config.json)
EXPOSE 8080

# Command to run when the container starts
CMD ["./server"]
