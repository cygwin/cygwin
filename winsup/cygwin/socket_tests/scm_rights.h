/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2018.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Supplementary program for Chapter 61 */

/* scm_rights.h

   Header file used by scm_rights_send.c and scm_rights_recv.c.
*/
#include <fcntl.h>
#include "unix_sockets.h"       /* Declares our unix*() socket functions */
#include "af_unix_hdr.h"

#define SOCK_PATH "scm_rights"
