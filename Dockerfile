# Multi-stage build for CppMMO project
# Stage 1: Build stage with necessary dependencies
FROM ubuntu:24.04 AS builder

# Set timezone to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libboost-all-dev \
    nlohmann-json3-dev \
    libhiredis-dev \
    redis-tools \
    curl \
    wget \
    gcc-13 \
    g++-13 \
    && rm -rf /var/lib/apt/lists/*

# Set GCC 13 as default (better C++20 support)
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

# Install Boost.Beast manually (since it's header-only but needs to be available)
RUN ldconfig

# Install spdlog from source
RUN cd /tmp && \
    git clone https://github.com/gabime/spdlog.git && \
    cd spdlog && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install

# Install redis-plus-plus from source
RUN cd /tmp && \
    git clone https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install

# Install flatbuffers from source - use v24.3.25 to match generated code
RUN cd /tmp && \
    git clone https://github.com/google/flatbuffers.git && \
    cd flatbuffers && \
    git checkout v24.3.25 && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install

# Install concurrentqueue (header-only library) manually
RUN cd /tmp && \
    wget https://raw.githubusercontent.com/cameron314/concurrentqueue/master/concurrentqueue.h \
    -O /usr/local/include/concurrentqueue.h && \
    wget https://raw.githubusercontent.com/cameron314/concurrentqueue/master/blockingconcurrentqueue.h \
    -O /usr/local/include/blockingconcurrentqueue.h

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Generate protocol headers with the correct FlatBuffers version
RUN cd /app/src/Common && flatc --cpp protocol.fbs

# Create build directory
RUN mkdir -p build

# Generate build files using CMake
WORKDIR /app/build
RUN rm -rf CMakeCache.txt CMakeFiles/ && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_CXX_STANDARD=20

# Build the project
RUN cmake --build . --config Release --parallel $(nproc)

# Stage 2: Runtime stage with minimal dependencies
FROM ubuntu:24.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.83.0 \
    libboost-thread1.83.0 \
    libboost-program-options1.83.0 \
    libboost-filesystem1.83.0 \
    libhiredis1.0.0 || \
    apt-get install -y \
    libssl3 \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-program-options-dev \
    libboost-filesystem-dev \
    libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy redis++ library from builder stage
COPY --from=builder /usr/local/lib/libredis++.so.1 /usr/local/lib/
COPY --from=builder /usr/local/lib/libspdlog.a /usr/local/lib/
COPY --from=builder /usr/local/lib/libflatbuffers.a /usr/local/lib/
COPY --from=builder /usr/local/bin/flatc /usr/local/bin/
RUN ldconfig

# Create app directory
WORKDIR /app

# Copy built executable from builder stage
COPY --from=builder /app/build/bin/CppMMO_Deployment /app/CppMMO_Deployment

# Copy logs directory
RUN mkdir -p /app/logs

# Expose port for the game server
EXPOSE 8080

# Set executable permissions
RUN chmod +x /app/CppMMO_Deployment

# Run the application
CMD ["./CppMMO_Deployment"]