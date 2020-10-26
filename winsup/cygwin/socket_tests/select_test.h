/* Header for select_sv.c and select_cl.c */

#include "af_unix_hdr.h"
#include <sys/select.h>
#include <fcntl.h>

#define SV_SOCK_PATH "/tmp/select"

#define BUF_SIZE 65527   /* MAX_AF_PKT_LEN - sizeof (af_unix_pkt_hdr_t) */

#define BACKLOG 5
