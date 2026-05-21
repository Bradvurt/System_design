# ---- Build Stage: uses userver-full prebuilt base (includes RabbitMQ) ----
FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS builder

# Copy source code
COPY . /src
WORKDIR /src/build

# Configure and build; point CMake to userver installation
RUN cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=/userver \
          -DUSERVER_FEATURE_RABBITMQ=ON \
          .. && \
    cmake --build . --parallel $(nproc)

# ---- Runtime Stage: minimal Ubuntu ----
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libssl3 libcurl4 \
    libboost-filesystem1.74.0 libboost-program-options1.74.0 \
    libfmt8 libyaml-cpp0.7 libgoogle-glog0.5 \
    libbson-1.0-0 libmongoc-1.0-0 librabbitmq4 \
    && rm -rf /var/lib/apt/lists/*

# Copy compiled binary and config
COPY --from=builder /src/build/hotel_booking_lab6 /app/hotel_booking_lab6
COPY config/config.yaml /app/config/config.yaml

WORKDIR /app
EXPOSE 8080

ENTRYPOINT ["./hotel_booking_lab6"]
CMD ["--config", "config/config.yaml"]