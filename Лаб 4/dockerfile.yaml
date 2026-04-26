FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

RUN printf '%s\n' \
    'deb http://archive.ubuntu.com/ubuntu jammy main universe multiverse restricted' \
    'deb http://archive.ubuntu.com/ubuntu jammy-updates main universe multiverse restricted' \
    'deb http://security.ubuntu.com/ubuntu jammy-security main universe multiverse restricted' \
    > /etc/apt/sources.list \
    && rm -rf /etc/apt/sources.list.d/* \
    && apt-get update -o Acquire::Retries=3 -o Acquire::http::Timeout=20 \
    && apt-get install -y --no-install-recommends \
    curl \
    libboost-all-dev \
    libhowardhinnant-date-dev \
    libbson-dev \
    libmongoc-dev \
    && ln -sf /usr/include/libbson-1.0/bson /usr/include/bson \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app

RUN cmake -S /app -B /app/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSERVER_FEATURE_TESTSUITE=OFF \
    -DUSERVER_FEATURE_GRPC=OFF \
    && cmake --build /app/build --target hotel_booking_lab4 -j 1

EXPOSE 8080

CMD ["/app/build/hotel_booking_lab4", "--config", "/app/config/static_config.yaml"]
