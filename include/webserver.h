#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdint.h>

int run_server(uint16_t port, const char *shutdown_token);

#endif
