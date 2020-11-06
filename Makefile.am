SUBDIRS = \
	src

# Note: specifying output into a different directory
bin_PROGRAMS = build/rmp

build_rmp_SOURCES = \
	src/base.cpp \
	src/config.h

# Note that library order matters
build_rmp_LDADD = \
	src/app/libapplication.a \
	src/inc/libradweb.a \
	src/inc/libfaad/libfaad.a \
	src/inc/alac/libalac.a \
	src/inc/zlib/libzlib.a \
	src/inc/sqlite3/libsqlite3.a \
	src/inc/microhttpd/libmircohttpd.a \
	src/inc/id3v2/libid3v2.a \
	src/inc/sha1/libsha1.a \
	src/inc/fft/libfft.a \
	src/inc/cie/libcie.a \
	mpg123/libmpg123.a \
	curl/libcurl.a

# Set capabilities (execute "make install-strip" as root)
install-exec-hook:
	setcap cap_ipc_lock,cap_net_bind_service,cap_sys_admin,cap_sys_nice,cap_sys_rawio,cap_net_admin=+ep $(DESTDIR)$(bindir)/rmp
	getcap $(DESTDIR)$(bindir)/rmp

AM_CXXFLAGS = -std=c++0x

