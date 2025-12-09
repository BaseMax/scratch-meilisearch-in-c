FROM alpine:latest

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make \
    bash \
    curl \
    nc

WORKDIR /app

COPY scratch-meilisearch.c .
COPY wait-for-it.sh .

RUN chmod +x wait-for-it.sh

RUN gcc -Wall -Wextra -std=c11 -O2 scratch-meilisearch.c -o scratch-meilisearch

ENV MEILI_HOST=meilisearch
ENV MEILI_PORT=7700

ENTRYPOINT ["sh", "-c", "./wait-for-it.sh ${MEILI_HOST}:${MEILI_PORT} -- ./scratch-meilisearch -h ${MEILI_HOST} -p ${MEILI_PORT}"]
