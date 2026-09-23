FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git curl zip unzip tar pkg-config ca-certificates \
    libssl-dev libjsoncpp-dev libpq-dev uuid-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git . \
    && ./bootstrap-vcpkg.sh -disableMetrics

WORKDIR /src
COPY . .
RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_MANIFEST_MODE=ON \
    && cmake --build build --config Release --target kanimart -j2

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 libpq5 libpq-dev libjsoncpp25 uuid-runtime zlib1g python3 postgresql-client ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/build/kanimart /app/kanimart
COPY --from=builder /src/frontend /app/frontend
COPY --from=builder /src/db /app/db
COPY docker/entrypoint.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh

ENV PORT=8080
EXPOSE 8080
CMD ["/app/entrypoint.sh"]
