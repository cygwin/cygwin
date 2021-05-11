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

6. fork/socketpair test.

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

7. select test.  In two terminals:

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

8. Ancillary data test (SCM_CREDENTIALS).  In two terminals:

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

   If we use -n in the send program, credentials will be sent even
   though the caller didn't specify control message data.

   scm_cred_send can also specify credentials:

   $ ./scm_cred_send.exe data 1 3 5

   This should fail with EPERM if the specified credentials are not
   the actual credentials of the sender, unless the sender is an
   administrator.  In that case the specified pid must be the pid of
   an existing process, but the uid and gid can be arbitrary.

9. Ancillary data test (SCM_RIGHTS, disk file descriptor).
   In two terminals:

   # Terminal 1:
   $ ./scm_rights_recv.exe

   # Terminal 2:
   $ ./scm_rights_send.exe <some disk file>
   Sending data = 12345
   Sending FD 3
   sendmsg() returned 4

   # Terminal 1 should now show:
   recvmsg() returned 4
   Received data = 12345
   Received FD 5
   <contents of some disk file>

10. Ancillary data test (SCM_RIGHTS, socket descriptor).

   $ ./is_seqnum_v3_sv.exe &
   [1] 8880

   $ ./is_seqnum_v2_cl.exe localhost
   Connection from (<host>, <port>)
   Sending fd 4 to child
   Sequence number: 0

11. Ancillary data test (SCM_RIGHTS, pty slave descriptor).
    send_pty_slave creates pty pair and a shell subprocess connected
    to the slave.  It sends the slave descriptor over an AF_UNIX
    socket to recv_pty_slave.  It then monitors its stdin and the pty
    master for input.  Anything it reads from stdin is written to the
    pty master (and so read by the shell).  Anything it reads from the
    pty master is written to stdout.  This is normally just the shell
    output.  But recv_pty_slave writes "hello" to the slave and so is
    read by send_pty_slave and written to stdout as though it were
    written by the shell.

    In two terminals:

    # Terminal 1:
    $ ./recv_pty_slave.exe
    Waiting for sender to connect and send descriptor...

    # Terminal 2:
    $ ./send_pty_slave.exe
    hello

    #Terminal 1 now shows:
    $ ./recv_pty_slave.exe
    Waiting for sender to connect and send descriptor...
    Received descriptor 5.
    Writing "hello" to that descriptor.
    This should appear in the other terminal as though it were output by the shell.

    Can now exit the shell in terminal 2.

    To test all this when the pty is connected to a pseudo terminal,
    set SHELL=cmd before running send_pty_slave.  Terminal 2 then
    looks like this:

    $ SHELL=cmd ./send_pty_slave.exe
    hello
    Microsoft Windows [Version 10.0.18363.1256]
    (c) 2019 Microsoft Corporation. All rights reserved.

    C:\Users\kbrown\src\cygdll\af_unix\winsup\cygwin\socket_tests>exit

12. Ancillary data test (SCM_RIGHTS, pty master descriptor).
    send_pty_master creates pty pair and a shell subprocess connected
    to the slave.  It then does the same as in 11, except that it
    sends the master descriptor instead of the slave descriptor.
    recv_pty_master writes "ps\n" to the received master fd.  The
    shell created by send_pty_master reads and executes this.

    In two terminals:

    # Terminal 1:
    $ ./recv_pty_master.exe
    Waiting for sender to connect and send descriptor...

    # Terminal 2:
    $ ./send_pty_master.exe
    ps

$ ps
      PID    PPID    PGID     WINPID   TTY         UID    STIME COMMAND
      934     933     934     138392  pty2      197609 13:47:22 /usr/bin/bash
      937     934     937     109052  pty2      197609 13:47:22 /usr/bin/ps
      887     886     887      51496  pty1      197609 13:11:25 /usr/bin/bash
      875     874     875      30396  pty0      197609 13:11:21 /usr/bin/bash
      874       1     874      23516  ?         197609 13:11:21 /usr/bin/mintty
      886       1     886     118428  ?         197609 13:11:25 /usr/bin/mintty
      933     887     933      59856  pty1      197609 13:47:22 /home/kbrown/src/cygdll/af_unix/winsup/cygwin/socket_tests/send_pty_master
      932     875     932     115304  pty0      197609 13:46:30 /home/kbrown/src/cygdll/af_unix/winsup/cygwin/socket_tests/recv_pty_master

      [Why is ps echoed twice?]


    #Terminal 1 now shows:
    $ ./recv_pty_master.exe
    Waiting for sender to connect and send descriptor...
    Received descriptor 5.
    Writing "ps" to that descriptor.
    This should appear in the other terminal
    and be executed by the shell running there.
    Waiting for sender to finish...

    Can now exit the shell in terminal 2 and both programs exit.

    This doesn't work if we use SHELL=cmd in terminal 2.  "ps" gets
    echoed but not executed.  I'm not sure if we should expect it to
    work.
