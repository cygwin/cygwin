include Makefile.inc

CFLAGS += -Ilib

AF_UNIX_LIB = libafunix.a

AF_UNIX_HDR = lib/af_unix_hdr.h

EXE = ud_ucase_sv ud_ucase_cl \
      us_xfr_cl us_xfr_sv \
      us_xfr_v2_cl us_xfr_v2_sv \
      scm_cred_recv scm_cred_send \
      scm_rights_recv scm_rights_send \
      scm_multi_recv scm_multi_send \
      us_abstract_bind \
      waitall_sv waitall_cl \
      readv_socket writev_socket \
      msg_peek_sv msg_peek_cl \
      fork_socketpair \
      select_sv select_cl \
      is_seqnum_v2_sv is_seqnum_v3_sv is_seqnum_v2_cl \
      recv_pty_slave send_pty_slave \
      recv_pty_master send_pty_master

all: ${EXE}

${EXE}: ${AF_UNIX_LIB}		# True as a rough approximation

${AF_UNIX_LIB}:
	cd lib; ${MAKE}

*.o: ${AF_UNIX_HDR}

scm_cred_recv.o scm_cred_send.o: scm_cred.h

scm_rights_recv.o scm_rights_send.o: scm_rights.h

scm_multi_recv.o scm_multi_send.o: scm_multi.h

us_xfr_sv.o us_xfr_cl.o: us_xfr.h 

us_xfr_v2_sv.o us_xfr_v2_cl.o: us_xfr_v2.h 

ud_ucase_sv.o ud_ucase_cl.o: ud_ucase.h

waitall_sv.o waitall_cl.o: waitall.h

readv_socket.o writev_socket.o: scatter_gather.h

msg_peek_sv.o msg_peek_cl.o: msg_peek.h

select_sv.o select_cl.o: select_test.h

is_seqnum_v2_sv.o is_seqnum_v3_sv.o is_seqnum_v2_cl.o: is_seqnum_v2.h

recv_pty_slave.o send_pty_slave.o: pty_slave.h

recv_pty_master.o send_pty_master.o: pty_master.h

clean:
	cd lib; ${MAKE} clean
	${RM} *.exe *.o ${AF_UNIX_LIB}
