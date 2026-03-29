FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libssl-dev \
    unixodbc-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel

EXPOSE 8080

WORKDIR /app/build
CMD ["./hotel_booking", "--config=/app/build/config.yaml"]
