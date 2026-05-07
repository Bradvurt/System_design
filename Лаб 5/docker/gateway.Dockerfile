FROM nginx:alpine

ARG SWAGGER_UI_VERSION=5.17.14

RUN apk add --no-cache curl unzip \
    && curl -fsSL "https://github.com/swagger-api/swagger-ui/archive/refs/tags/v${SWAGGER_UI_VERSION}.zip" -o /tmp/swagger-ui.zip \
    && unzip -q /tmp/swagger-ui.zip -d /tmp \
    && mv "/tmp/swagger-ui-${SWAGGER_UI_VERSION}/dist" /usr/share/nginx/html/swagger \
    && rm -rf /tmp/swagger-ui.zip "/tmp/swagger-ui-${SWAGGER_UI_VERSION}" \
    && sed -i 's|https://petstore.swagger.io/v2/swagger.json|/openapi.yaml|g' /usr/share/nginx/html/swagger/swagger-initializer.js

COPY docs/openapi.yaml /openapi/openapi.yaml
COPY docker/gateway-nginx.conf /etc/nginx/conf.d/default.conf
