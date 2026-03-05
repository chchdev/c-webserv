#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "webserver.h"

static uint16_t parse_port_or_default(const char *value) {
    char *end_ptr = NULL;
    errno = 0;
    unsigned long parsed = strtoul(value, &end_ptr, 10);

    if (errno != 0 || end_ptr == value || *end_ptr != '\0' || parsed == 0 || parsed > 65535) {
        fprintf(stderr, "Invalid port '%s'. Using default port 8080.\n", value);
        return 8080;
    }

    return (uint16_t)parsed;
}

int main(int argc, char **argv) {
    uint16_t port = 8080;
    const char *shutdown_token = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--shutdown-token") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --shutdown-token\n");
                return 1;
            }

            shutdown_token = argv[i + 1];
            i++;
            continue;
        }

        port = parse_port_or_default(argv[i]);
    }

    return run_server(port, shutdown_token);
}
