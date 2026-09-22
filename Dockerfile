# Stage 1: Build Environment
FROM debian:bookworm AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libsqlite3-dev \
    libsodium-dev \
    libspdlog-dev \
    nlohmann-json3-dev \
    libgtest-dev \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --config Release

# Stage 2: Minimal Runtime Container
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libsqlite3-0 \
    libsodium23 \
    libspdlog1.10 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/ddmart

# Copy compiled standalone binary and static assets
COPY --from=builder /app/build/ddmart /opt/ddmart/bin/ddmart
COPY --from=builder /app/public /opt/ddmart/public
COPY --from=builder /app/db /opt/ddmart/db

ENV PORT=8080
ENV HOST=0.0.0.0
ENV DB_PATH=/opt/ddmart/data/ddmart.db

RUN mkdir -p /opt/ddmart/data && chmod -R 755 /opt/ddmart

EXPOSE 8080

CMD ["/opt/ddmart/bin/ddmart"]
