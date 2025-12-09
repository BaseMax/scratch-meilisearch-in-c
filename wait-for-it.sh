CMD sh -c 'until nc -z meilisearch 7700; do echo "Waiting for Meilisearch..."; sleep 1; done; ./scratch-meilisearch -h meilisearch -p 7700'
