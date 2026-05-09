FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

RUN apt-get update -o Acquire::Retries=3 -o Acquire::http::Timeout=20 \
    && apt-get install -y --no-install-recommends \
        curl \
        libboost-all-dev \
        libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN cmake -S /app -B /app/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSERVER_FEATURE_TESTSUITE=OFF \
    && cmake --build /app/build --target hotel_booking_lab5 -j 1

EXPOSE 8080
CMD ["/app/build/hotel_booking_lab5", "--config", "/app/config/static_config.yaml"]