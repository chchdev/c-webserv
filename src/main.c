#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "webserver.h"

static uint16_t parse_port_or_default(int argc, char **argv) {
    if (argc < 2) {
        return 8080;
    }

    char *end_ptr = NULL;
    errno = 0;
    unsigned long value = strtoul(argv[1], &end_ptr, 10);

    if (errno != 0 || end_ptr == argv[1] || *end_ptr != '\0' || value == 0 || value > 65535) {
        fprintf(stderr, "Invalid port '%s'. Using default port 8080.\n", argv[1]);
        return 8080;
    }

    return (uint16_t)value;
}

int main(int argc, char **argv) {
    uint16_t port = parse_port_or_default(argc, argv);
    return run_server(port);
}
