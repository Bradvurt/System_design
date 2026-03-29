FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

RUN apt-get update && apt-get install -y     cmake     build-essential     libssl-dev     unixodbc-dev     && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release     && cmake --build build --parallel

# Keep the runtime config explicit so container start does not depend on CMake copy rules.
COPY config/config.yaml /app/build/config.yaml

EXPOSE 8080

CMD ["/app/build/hotel_booking", "--config=/app/build/config.yaml"]
