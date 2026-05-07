FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

RUN apt-get update && apt-get install -y --no-install-recommends curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app

RUN cmake -S /app -B /app/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /app/build --target hotel_booking_lab2 hotel_booking_lab2_service_tests -j 2 \
    && ctest --test-dir /app/build --output-on-failure

EXPOSE 8080

CMD ["/app/build/hotel_booking_lab2", "--config", "/app/config/static_config.yaml"]
