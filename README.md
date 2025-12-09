# scratch-meilisearch-in-c

A **from-scratch Meilisearch client written in pure C**. No dependencies, no external libraries, just POSIX sockets and standard C.

This is a minimal, educational, and fully functional HTTP client that talks directly to a Meilisearch server using raw TCP sockets and handcrafted HTTP/1.1 requests.

Perfect for learning how HTTP works under the hood, systems programming, or just for fun!

## Features

- Pure C (C11) **zero dependencies**
- Full HTTP/1.1 request building from scratch
- Interactive REPL mode (`meili>`)
- Non-interactive one-shot commands
- Supports most common Meilisearch operations:
  - `list_indexes`
  - `create_index`, `delete_index`
  - `add_docs`, `get_docs`, `search`
  - `update_settings`
- JSON string escaping (for queries)
- API key authentication (`-a`)
- Raw HTTP mode for advanced use
- Works perfectly with Docker + Meilisearch

## Interactive mode using docker compose run

```
docker compose run --rm meili-client
```

- Opens the REPL.
- You can type commands like list_indexes, create_index movies title, etc.
- Exits when you type quit or exit.

### Run one-shot commands

```
docker compose run --rm meili-client ./scratch-meilisearch -h meilisearch list_indexes
```

## Quick Start (Docker Recommended)

Clone the repo and run with Docker Compose:

```bash
git clone https://github.com/BaseMax/scratch-meilisearch-in-c.git
cd scratch-meilisearch-in-c
docker compose up --build
````

You’ll get a live interactive terminal:

```
Connected to Meilisearch at meilisearch:7700. Enter commands (type 'quit' or 'exit' to stop).
meili> list_indexes
meili> create_index movies title
meili> add_docs movies [{"id":1,"title":"The Matrix"},{"id":2,"title":"Inception"}]
meili> search movies matrix
```

### Docker Arguments

You can customize the Meilisearch hostname, port, and master key using environment variables:

* `MEILI_HOST`: hostname of Meilisearch (default: `meilisearch`)
* `MEILI_PORT`: Meilisearch port (default: `7700`)
* `MEILI_MASTER_KEY`: optional master key for authentication

Example:

```bash
docker compose run -e MEILI_HOST=127.0.0.1 -e MEILI_PORT=7700 -e MEILI_MASTER_KEY=your_master_key meili-client
```

These variables are automatically passed into `wait-for-it.sh` and the client.

---

## Manual Build & Run (Linux/macOS/WSL)

```bash
gcc -Wall -Wextra -std=c11 -O2 scratch-meilisearch.c -o scratch-meilisearch
./scratch-meilisearch
```

Or with custom host/port/key:

```bash
./scratch-meilisearch -h 127.0.0.1 -p 7700 -a your_master_key
```

---

## Supported Commands

| Command                             | Example                                    | Description                   |
| ----------------------------------- | ------------------------------------------ | ----------------------------- |
| `list_indexes`                      | `list_indexes`                             | List all indexes              |
| `create_index <uid> [pk]`           | `create_index movies title`                | Create index with primary key |
| `get_index <uid>`                   | `get_index movies`                         | Get index info                |
| `delete_index <uid>`                | `delete_index movies`                      | Delete index                  |
| `add_docs <index> <json>`           | `add_docs movies [{"id":1,"title":"..."}]` | Add documents                 |
| `search <index> <query>`            | `search movies inception`                  | Search documents              |
| `get_docs <index> [limit] [offset]` | `get_docs movies 10 0`                     | Retrieve documents            |
| `update_settings <index> <json>`    | `update_settings movies {...}`             | Update index settings         |

Or use **raw HTTP** mode:

```bash
GET /indexes
POST /indexes/movies/documents '{"documents":[...]}'
```

---

## Project Structure

```
scratch-meilisearch-in-c/
├── scratch-meilisearch.c     ← Main source (pure C)
├── Dockerfile                ← Builds the client
├── docker-compose.yml        ← Runs Meilisearch + client
├── wait-for-it.sh            ← Waits for Meilisearch to be ready
├── LICENSE                   ← MIT License
└── README.md                 ← This file
```

---

## Run with Docker Compose

```bash
docker compose up --build
```

One-shot examples:

```bash
docker compose run --rm meili-client ./scratch-meilisearch -h meilisearch create_index books
docker compose run --rm meili-client ./scratch-meilisearch -h meilisearch search books "dystopia"
```

You can also pass custom host/port/master key:

```bash
docker compose run --rm -e MEILI_HOST=127.0.0.1 -e MEILI_PORT=7700 -e MEILI_MASTER_KEY=your_master_key meili-client
```

---

## Author

**Seyyed Ali Mohammadiyeh** ([@BaseMax](https://github.com/BaseMax))

---

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.
