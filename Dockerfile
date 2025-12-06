FROM alpine:latest

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make

WORKDIR /app
COPY scratch-meilisearch.c .

RUN gcc -Wall -Wextra -std=c11 -O2 scratch-meilisearch.c -o scratch-meilisearch

CMD ["./scratch-meilisearch", "-h", "meilisearch", "-p", "7700"]
