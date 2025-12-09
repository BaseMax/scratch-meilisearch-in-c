FROM alpine:latest

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make \
    bash \
    curl

WORKDIR /app
COPY scratch-meilisearch.c .
COPY wait-for-it.sh .

CMD ["wait-for-it.sh", "meilisearch:7700", "--", "./scratch-meilisearch", "-h", "meilisearch", "-p", "7700"]
RUN gcc -Wall -Wextra -std=c11 -O2 scratch-meilisearch.c -o scratch-meilisearch
