#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <strings.h>
#include <ctype.h>

#define BUFFER_SIZE 4096
#define MAX_ARGS 64
#define DEFAULT_PORT 7700

int connect_to_meili(const char* host, int port);
char* json_escape(const char* str);
int parse_command(char* input, char** args, int max_args);
int build_request(char** args, int argc, char* method_out, char* path_out, char* body_out, size_t buf_size);
size_t serialize_http_request(char* buffer, size_t buf_size, const char* method, const char* path, const char* body, const char* host, const char* api_key);
ssize_t read_bytes(int sock, char* buf, size_t len);
char* read_line(int sock);
void parse_and_print_http_response(int sock);
void usage(void);

int connect_to_meili(const char* host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

char* json_escape(const char* str) {
    size_t len = strlen(str);
    char* escaped = malloc(len * 6 + 1);
    if (!escaped) return NULL;
    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == '\"') {
            strcpy(escaped + pos, "\\\"");
            pos += 2;
        } else if (c == '\\') {
            strcpy(escaped + pos, "\\\\");
            pos += 2;
        } else if (c == '\b') {
            strcpy(escaped + pos, "\\b");
            pos += 2;
        } else if (c == '\f') {
            strcpy(escaped + pos, "\\f");
            pos += 2;
        } else if (c == '\n') {
            strcpy(escaped + pos, "\\n");
            pos += 2;
        } else if (c == '\r') {
            strcpy(escaped + pos, "\\r");
            pos += 2;
        } else if (c == '\t') {
            strcpy(escaped + pos, "\\t");
            pos += 2;
        } else if (!isprint(c)) {
            pos += sprintf(escaped + pos, "\\u%04x", (unsigned char)c);
        } else {
            escaped[pos++] = c;
        }
    }
    escaped[pos] = '\0';
    return escaped;
}

int parse_command(char* input, char** args, int max_args) {
    int argc = 0;
    char* p = input;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        if (*p == '"') {
            p++;
            args[argc] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = '\0';
        } else {
            args[argc] = p;
            while (*p && *p != ' ') p++;
            if (*p == ' ') *p++ = '\0';
        }

        argc++;
        if (argc >= max_args) break;
    }
    return argc;
}

int build_request(char** args, int argc, char* method_out, char* path_out, char* body_out, size_t buf_size) {
    if (argc < 1) return -1;

    char* cmd = args[0];
    *body_out = '\0';

    if (strcasecmp(cmd, "list_indexes") == 0) {
        strcpy(method_out, "GET");
        strcpy(path_out, "/indexes");
    } else if (strcasecmp(cmd, "get_index") == 0 && argc >= 2) {
        strcpy(method_out, "GET");
        snprintf(path_out, buf_size, "/indexes/%s", args[1]);
    } else if (strcasecmp(cmd, "create_index") == 0 && argc >= 2) {
        strcpy(method_out, "POST");
        strcpy(path_out, "/indexes");
        char* pk = (argc >= 3) ? args[2] : NULL;
        if (pk) {
            snprintf(body_out, buf_size, "{\"uid\":\"%s\",\"primaryKey\":\"%s\"}", args[1], pk);
        } else {
            snprintf(body_out, buf_size, "{\"uid\":\"%s\"}", args[1]);
        }
    } else if (strcasecmp(cmd, "delete_index") == 0 && argc >= 2) {
        strcpy(method_out, "DELETE");
        snprintf(path_out, buf_size, "/indexes/%s", args[1]);
    } else if (strcasecmp(cmd, "add_docs") == 0 && argc >= 3) {
        strcpy(method_out, "POST");
        snprintf(path_out, buf_size, "/indexes/%s/documents", args[1]);
        strncpy(body_out, args[2], buf_size - 1);
    } else if (strcasecmp(cmd, "search") == 0 && argc >= 3) {
        strcpy(method_out, "POST");
        snprintf(path_out, buf_size, "/indexes/%s/search", args[1]);
        char* esc_query = json_escape(args[2]);
        if (!esc_query) return -1;
        snprintf(body_out, buf_size, "{\"q\":\"%s\"}", esc_query);
        free(esc_query);
    } else if (strcasecmp(cmd, "get_docs") == 0 && argc >= 2) {
        strcpy(method_out, "GET");
        snprintf(path_out, buf_size, "/indexes/%s/documents", args[1]);
        char query[BUFFER_SIZE] = "";
        if (argc >= 3) snprintf(query, sizeof(query), "?limit=%s", args[2]);
        if (argc >= 4) snprintf(query + strlen(query), sizeof(query) - strlen(query), "&offset=%s", args[3]);
        strncat(path_out, query, buf_size - strlen(path_out) - 1);
    } else if (strcasecmp(cmd, "update_settings") == 0 && argc >= 3) {
        strcpy(method_out, "PATCH");
        snprintf(path_out, buf_size, "/indexes/%s/settings", args[1]);
        strncpy(body_out, args[2], buf_size - 1);
    } else {
        if (argc >= 2) {
            strcpy(method_out, args[0]);
            strcpy(path_out, args[1]);
            if (argc >= 3) strncpy(body_out, args[2], buf_size - 1);
            return 0;
        }
        return -1;
    }
    return 0;
}

size_t serialize_http_request(char* buffer, size_t buf_size, const char* method, const char* path, const char* body, const char* host, const char* api_key) {
    size_t len = 0;
    len += snprintf(buffer + len, buf_size - len, "%s %s HTTP/1.1\r\n", method, path);
    len += snprintf(buffer + len, buf_size - len, "Host: %s\r\n", host);
    if (api_key) {
        len += snprintf(buffer + len, buf_size - len, "Authorization: Bearer %s\r\n", api_key);
    }
    if (body && *body) {
        size_t body_len = strlen(body);
        len += snprintf(buffer + len, buf_size - len, "Content-Type: application/json\r\n");
        len += snprintf(buffer + len, buf_size - len, "Content-Length: %zu\r\n", body_len);
    }
    len += snprintf(buffer + len, buf_size - len, "\r\n");
    if (body && *body) {
        memcpy(buffer + len, body, strlen(body));
        len += strlen(body);
    }
    return len;
}

ssize_t read_bytes(int sock, char* buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t r = recv(sock, buf + total, len - total, 0);
        if (r <= 0) {
            if (r < 0) perror("recv");
            return r;
        }
        total += r;
    }
    return total;
}

char* read_line(int sock) {
    static char buf[BUFFER_SIZE];
    size_t pos = 0;
    while (pos < BUFFER_SIZE - 1) {
        char c;
        ssize_t r = recv(sock, &c, 1, 0);
        if (r <= 0) {
            if (r < 0) perror("recv");
            return NULL;
        }
        buf[pos++] = c;
        if (pos >= 2 && buf[pos - 2] == '\r' && buf[pos - 1] == '\n') {
            buf[pos - 2] = '\0';
            return buf;
        }
    }
    fprintf(stderr, "Line too long\n");
    return NULL;
}

void parse_and_print_http_response(int sock) {
    char* line = read_line(sock);
    if (!line) return;
    printf("%s\n", line);

    size_t content_length = 0;
    while ((line = read_line(sock))) {
        if (*line == '\0') break;
        printf("%s\n", line);
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            content_length = atol(line + 15);
        }
    }

    if (content_length > 0) {
        char* body = malloc(content_length + 1);
        if (!body) {
            fprintf(stderr, "Malloc failed\n");
            return;
        }
        if (read_bytes(sock, body, content_length) != content_length) {
            free(body);
            return;
        }
        body[content_length] = '\0';
        printf("\n%s\n", body);
        free(body);
    }
}

void usage(void) {
    printf("Usage: meili-client [options] [cmd [arg [arg ...]]]\n"
           "A simple Meilisearch client.\n\n"
           "Options:\n"
           "  -h <hostname>      Server hostname (default: 127.0.0.1)\n"
           "  -p <port>          Server port (default: 7700)\n"
           "  -a <api_key>       API key for authentication\n\n"
           "Supported commands:\n"
           "  list_indexes\n"
           "  get_index <uid>\n"
           "  create_index <uid> [primary_key]\n"
           "  delete_index <uid>\n"
           "  add_docs <index> <json_array>\n"
           "  search <index> <query>\n"
           "  get_docs <index> [limit] [offset]\n"
           "  update_settings <index> <json>\n"
           "  Or raw: GET/POST/etc <path> [body]\n\n"
           "If command arguments are provided, execute the command and exit.\n"
           "Otherwise, enter interactive mode where you can type commands.\n"
           "In interactive mode, type 'quit' or 'exit' to stop.\n");
    exit(1);
}

int main(int argc, char** argv) {
    char* host = "127.0.0.1";
    int port = DEFAULT_PORT;
    char* api_key = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "h:p:a:")) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                if (port <= 0) {
                    fprintf(stderr, "Invalid port\n");
                    usage();
                }
                break;
            case 'a':
                api_key = optarg;
                break;
            default:
                usage();
        }
    }

    int sock = connect_to_meili(host, port);
    if (sock < 0) {
        fprintf(stderr, "Failed to connect to Meilisearch at %s:%d\n", host, port);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    char method[16], path[BUFFER_SIZE], body[BUFFER_SIZE];

    if (optind < argc) {
        int cmd_argc = argc - optind;
        char** cmd_args = argv + optind;

        if (build_request(cmd_args, cmd_argc, method, path, body, sizeof(body)) < 0) {
            fprintf(stderr, "Invalid command\n");
            close(sock);
            return 1;
        }

        size_t len = serialize_http_request(buffer, sizeof(buffer), method, path, body, host, api_key);
        if (len == 0 || len >= sizeof(buffer)) {
            fprintf(stderr, "Request too long\n");
            close(sock);
            return 1;
        }

        if (send(sock, buffer, len, 0) < 0) {
            perror("send");
            close(sock);
            return 1;
        }

        parse_and_print_http_response(sock);
        close(sock);
        return 0;
    } else {
        printf("Connected to Meilisearch at %s:%d. Enter commands (type 'quit' or 'exit' to stop).\n", host, port);

        char input[BUFFER_SIZE];
        char* args[MAX_ARGS];

        while (1) {
            printf("meili> ");
            fflush(stdout);

            if (fgets(input, sizeof(input), stdin) == NULL) {
                break;
            }

            input[strcspn(input, "\n")] = '\0';

            if (strlen(input) == 0) continue;

            if (strcasecmp(input, "quit") == 0 || strcasecmp(input, "exit") == 0) {
                break;
            }

            int pargc = parse_command(input, args, MAX_ARGS);
            if (pargc == 0) continue;

            if (build_request(args, pargc, method, path, body, sizeof(body)) < 0) {
                fprintf(stderr, "Invalid command\n");
                continue;
            }

            size_t len = serialize_http_request(buffer, sizeof(buffer), method, path, body, host, api_key);
            if (len == 0 || len >= sizeof(buffer)) {
                fprintf(stderr, "Request too long\n");
                continue;
            }

            if (send(sock, buffer, len, 0) < 0) {
                perror("send");
                break;
            }

            parse_and_print_http_response(sock);
        }

        close(sock);
        printf("Disconnected.\n");
        return 0;
    }
}
