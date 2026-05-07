FROM ghcr.io/userver-framework/ubuntu-24.04-userver:latest AS build
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel

FROM ghcr.io/userver-framework/ubuntu-24.04-userver:latest
COPY --from=build /src/build/hotel_booking /usr/local/bin/hotel_booking
COPY config/config.yaml /etc/hotel_booking/config.yaml
EXPOSE 8080
CMD ["hotel_booking", "--config", "/etc/hotel_booking/config.yaml"]