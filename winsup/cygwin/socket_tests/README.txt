0. make

1. Server-client using read/write.

   $ cat *.c > a

   $ ./us_xfr_sv.exe > b&

   $ ./us_xfr_cl.exe < a

   $ kill %1

   $
   [1]+  Terminated              ./us_xfr_sv.exe > b

   $ diff a b

   $ rm a b

   Should be able to do same test with v2 versions.

2. Datagram server-client using sendto/recvfrom.

   $ ./ud_ucase_sv.exe &

   $ ./ud_ucase_cl.exe long message
   Server received 4 bytes from /tmp/ud_ucase_cl.925
   Response 1: LONG
   Server received 7 bytes from /tmp/ud_ucase_cl.925
   Response 2: MESSAGE

   $ ./ud_ucase_cl.exe 'long message'
   Server received 10 bytes from /tmp/ud_ucase_cl.926
   Response 1: LONG MESSA

   $ kill %1

3. MSG_WAITALL test.  In two terminals:

   # Terminal 1:
   $ ./waitall_sv.exe

   # Terminal 2:
   $ ./waitall_cl.exe
   abcd
   abcd

   [Should see this echoed in Terminal 1 after both lines have been
   typed.  Kill both programs with Ctrl-C.]

4. scatter-gather test.  In two terminals:

   # Terminal 1:
   $ ./readv_socket.exe

   # Terminal 2:
   $ ./writev_socket.exe
   wrote 148 bytes

   # Terminal 1 should now show:
   $ ./readv_socket.exe
   read 148 bytes
   0: The term buccaneer comes from the word boucan.
   1: A boucan is a wooden frame used for cooking meat.
   2: Buccaneer is the West Indies name for a pirate.

5. MSG_PEEK test.  In two terminals:

   # Terminal 1:
   $ ./msg_peek_sv.exe
   peeking...

   # Terminal 2:
   $ ./msg_peek_cl.exe
   hello

   # Terminal 1 should now show:
   $ ./msg_peek_sv.exe
   peeking...
   reading would yield 6 bytes: hello

   [After 1 second delay]

   reading...
   read 6 bytes: hello

   [Need to kill msg_peek_cl.]

6. SCM_CREDENTIALS test.  In two terminals:

   # Terminal 1:
   $ ./scm_cred_recv.exe

   # Terminal 2:
   $ ./scm_cred_send.exe
   Sending data = 12345
   Send credentials pid=234, uid=197609, gid=197121
   sendmsg() returned 4

   # Terminal 1 should now show:
   $ ./scm_cred_recv.exe
   recvmsg() returned 4
   Received data = 12345
   Received credentials pid=234, uid=197609, gid=197121
   Credentials from SO_PEERCRED: pid=234, euid=197609, egid=197121

   If use -d option in both programs to use datagrams, the last line
   instead reads:

   ERROR [EINVAL Invalid argument] getsockopt

   I think this is correct.  According to
   https://man7.org/linux/man-pages/man7/unix.7.html, SO_PEERCRED is
   not supported for datagram sockets unless they are created using
   socketpair.

   But if we use -n in the send program, we get

   ERROR [?UNKNOWN? No error] recvmsg

   in the recv program.  This needs to be fixed.  Set appropriate
   errno if recvmsg is expecting ancillary data but doesn't get it.

   scm_cred_send can also specify credentials:

   $ ./scm_cred_send.exe data 1 3 5

   This should fail with EPERM if the specified credentials are not
   the actual credentials of the sender, unless the sender is an
   administrator.  In that case the specified pid must be the pid of
   an existing process, but the uid and gid can be arbitrary.

7. fork/socketpair test.

   $ ./fork_socketpair.exe
   count = 0
   count = 1
   count = 2
   count = 3
   count = 4
   count = 5
   count = 6
   count = 7
   count = 8
   count = 9

8. select test.  In two terminals:

   # Terminal 1:
   $ ./select_sv
   waiting for connection request...

   # Terminal 2:
   $ ./select_cl
   waiting for socket to be ready for write...
   ready for write, writing until buffer full
   buffer full
   wrote 262108 bytes
   waiting for write ready again...
   ready for write, writing once more
   wrote 65527 more bytes for a total of 327635

   # Terminal 1 should now show:
   $ ./select_sv
   waiting for connection request...
   connection request received; accepting
   slowly reading from socket...
   read 327635 bytes
