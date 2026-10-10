# Stage 1: Build Environment
FROM debian:bookworm AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Ensure clean build directory to prevent host-container cache conflicts
RUN rm -rf build && cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    && cmake --build build --config Release

# Stage 2: Minimal Runtime Container
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
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
