FROM ubuntu:24.04

# No prompts interactivos.
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /app

# Dependencias.
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    g++ \
    git \
    make \
    python3 \
    python3-matplotlib \
    && rm -rf /var/lib/apt/lists/*

# Catch2 (tests).
RUN cd /tmp && \
    git clone --depth 1 --branch v3.4.0 https://github.com/catchorg/Catch2.git && \
    cd Catch2 && \
    mkdir build && cd build && \
    cmake -DBUILD_TESTING=OFF .. && \
    make -j$(nproc) && make install && \
    ldconfig && \
    cd / && rm -rf /tmp/Catch2

WORKDIR /workspace

CMD ["bash"]