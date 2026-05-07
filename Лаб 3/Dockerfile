FROM ghcr.io/userver-framework/ubuntu-24.04-userver:latest

RUN set -eux; \
    for i in 1 2 3 4 5; do \
        apt-get update && apt-get install -y --no-install-recommends \
            curl \
            ca-certificates \
            libhowardhinnant-date-dev \
            python3-venv \
        && break; \
        sleep 5; \
    done; \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN cmake -S /app -B /app/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /app/build --target hotel_booking_lab3 -j 1

EXPOSE 8080
ENV POSTGRES_DSN=postgresql://hotel:hotel@postgres:5432/hotel_booking
CMD ["/app/build/hotel_booking_lab3", "--config", "/app/config/static_config.yaml"]