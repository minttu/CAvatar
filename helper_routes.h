#ifndef CAVATAR_HELPER_ROUTES_H
#define CAVATAR_HELPER_ROUTES_H

#include <evhtp.h>

void route_404(evhtp_request_t *req);
void route_400(evhtp_request_t *req);
void serve_static(evhtp_request_t *req, char *file, char *type);

#endif