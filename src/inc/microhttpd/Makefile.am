noinst_LIBRARIES = libmircohttpd.a

libmircohttpd_a_SOURCES = \
	autoinit_funcs.h \
	base64.c \
	base64.h \
	basicauth.c \
	connection.c \
	connection.h \
	connection_https.c \
	connection_https.h \
	daemon.c \
	digestauth.c \
	internal.c \
	internal.h \
	md5.c \
	md5.h \
	memorypool.c \
	memorypool.h \
	mhd_assert.h \
	mhd_bithelpers.h \
	mhd_byteorder.h \
	mhd_compat.c \
	mhd_compat.h \
	MHD_config.h \
	mhd_itc.c \
	mhd_itc.h \
	mhd_itc_types.h \
	mhd_limits.h \
	mhd_locks.h \
	mhd_mono_clock.c \
	mhd_mono_clock.h \
	mhd_options.h \
	mhd_send.c \
	mhd_send.h \
	mhd_sockets.c \
	mhd_sockets.h \
	mhd_str.c \
	mhd_str.h \
	mhd_threads.c \
	mhd_threads.h \
	microhttpd2.h \
	microhttpd.h \
	microhttpd_tls.h \
	platform.h \
	postprocessor.c \
	reason_phrase.c \
	response.c \
	response.h \
	sha256.c \
	sha256.h \
	sysfdsetsize.c \
	sysfdsetsize.h \
	tsearch.c \
	tsearch.h

#AM_CFLAGS = -std=c99 -DUSE_SSL -DUSE_CYASSL -D_GNU_SOURCE -O3 -Wall -c -fmessage-length=0 -MMD -MP
AM_CFLAGS = -std=c99 -DUSE_SSL -DUSE_CYASSL -D_GNU_SOURCE

