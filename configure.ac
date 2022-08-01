AC_PREREQ([2.68])

# Get SVN version
define([SVN_VER], esyscmd([sh -c "svnversion|tr -d '\n'"]))

# Initial information about the project
AC_INIT([RMP], [SVN_VER], [info@dbrinkmeier.de])

# Check if the source folder is correct
AC_CONFIG_SRCDIR([src/base.cpp])

# Clear default compiler flags
dnl : ${CXXFLAGS=""}
dnl : ${CFLAGS=""}

# Check for C/C++ compiler and linker
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

# Automake initialisation (mandatory) and check for minimal automake API version 1.9
# Leave objects in sub directories
AM_INIT_AUTOMAKE([1.9] [subdir-objects])

# Use the C++ compiler for the following checks
AC_LANG([C++])

# Checks for default system types.
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_STAT

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_OFF_T

# Add configuration parameter (--enable-debug) to enable build with debug symbols
# --> To be use in Makefile.am (if DEBUG ...)
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug], [Enable debug build, default: no]),
[case "${enableval}" in
	yes) debug=true ;;
	no)  debug=false ;;
	*)   AC_MSG_ERROR([Bad value ${enableval} for --enable-debug]) ;;
esac], [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Add configuration parameter (--enable-curl-https) to enable IDN2 library neede by libcurl to support https
AC_ARG_ENABLE(curl_https,
AS_HELP_STRING([--enable-curl-https], [HTTPS support in static libcurl enabled, default: no]),
[case "${enableval}" in
	yes) curl_https=true ;;
	no)  curl_https=false ;;
	*)   AC_MSG_ERROR([Bad value ${enableval} for --enable-curl-https]) ;;
esac], [curl_https=false])
AM_CONDITIONAL(ENABLE_CURL_HTTPS, test x"$curl_https" = x"true")

# Checks for libraries
AC_CHECK_LIB([gnutls],[gnutls_handshake_description_get_name], [], [AC_MSG_ERROR([The GnuTLS (gnutls) library is required.])])
AC_CHECK_LIB([crypto],[CRYPTO_get_ex_new_index], [], [AC_MSG_ERROR([The Crypto (crypto) library is required.])])
AC_CHECK_LIB([asound],[snd_asoundlib_version], [], [AC_MSG_ERROR([The ALSA (asound) library is required.])])
AC_CHECK_LIB([png],[png_get_error_ptr], [], [AC_MSG_ERROR([The PNG (png) library is required.])])
AC_CHECK_LIB([exif],[exif_entry_get_value], [], [AC_MSG_ERROR([The Exif (exif) library is required.])])
AC_CHECK_LIB([FLAC],[FLAC__metadata_chain_delete], [], [AC_MSG_ERROR([The FLAC (FLAC) library is required.])])
AC_CHECK_LIB([jpeg],[jpeg_stdio_dest], [], [AC_MSG_ERROR([The JPEG (jpeg) library is required.])])
AC_CHECK_LIB([rt],[close], [], [AC_MSG_ERROR([The RT (rt) library is required.])])
AC_CHECK_LIB([ssl],[SSL_SRP_CTX_free], [], [AC_MSG_ERROR([The OpenSLL (ssl) library is required.])])
AC_CHECK_LIB([pthread],[pthread_self], [], [AC_MSG_ERROR([The Thread (pthread) library is required.])])
AC_CHECK_LIB([dl],[dlclose], [], [AC_MSG_ERROR([The DL (dl) library is required.])])
AC_CHECK_LIB([bfd],[bfd_get_stab_name], [], [AC_MSG_ERROR([The BFD (bfd) library is required.])])
AC_CHECK_LIB([iberty],[fnmatch], [], [AC_MSG_ERROR([The Iberty (iberty) library is required.])])
AC_CHECK_LIB([udev],[udev_device_unref], [], [AC_MSG_ERROR([The UDEV (udev) library is required.])])
AC_CHECK_LIB([cap],[cap_get_proc], [], [AC_MSG_ERROR([The Capabilities (cap) library is required.])])
AC_CHECK_LIB([avahi-common],[avahi_strerror], [], [AC_MSG_ERROR([The Avahi common (avahi-common) library is required.])])
AC_CHECK_LIB([avahi-client],[avahi_client_new], [], [AC_MSG_ERROR([The Avahi client (avahi-client) library is required.])])

if test x"$curl_https" = x"true"; then
AC_CHECK_LIB([idn2],[idn2_lookup_ul], [], [AC_MSG_ERROR([The IDN2 (idn2) library is required.])])
fi

# Checks for C++ STL header files.
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([iostream])
AC_CHECK_HEADERS([fstream])
AC_CHECK_HEADERS([sstream])
AC_CHECK_HEADERS([vector])
AC_CHECK_HEADERS([map])
AC_CHECK_HEADERS([memory])
AC_CHECK_HEADERS([mutex])
AC_CHECK_HEADERS([tuple])
AC_CHECK_HEADERS([complex])
AC_CHECK_HEADERS([valarray])
AC_CHECK_HEADERS([array])
AC_CHECK_HEADERS([random])
AC_CHECK_HEADERS([thread])
AC_CHECK_HEADERS([functional])
AC_CHECK_HEADERS([type_traits])
AC_CHECK_HEADERS([iomanip])
AC_CHECK_HEADERS([algorithm])
AC_CHECK_HEADERS([locale])
AC_CHECK_HEADERS([cstring])
AC_CHECK_HEADERS([cmath])
AC_CHECK_HEADERS([cstdarg])
AC_CHECK_HEADERS([csetjmp])
AC_CHECK_HEADERS([cwchar])

# Checks for C GLibC header files.
AC_CHECK_HEADERS([assert.h], [], [AC_MSG_ERROR([Required header not found <assert.h>])], [])
AC_CHECK_HEADERS([unistd.h], [], [AC_MSG_ERROR([Required header not found <unistd.h>])], [])
AC_CHECK_HEADERS([stdlib.h], [], [AC_MSG_ERROR([Required header not found <stdlib.h>])], [])
AC_CHECK_HEADERS([stdarg.h], [], [AC_MSG_ERROR([Required header not found <stdarg.h>])], [])
AC_CHECK_HEADERS([sys/types.h], [], [AC_MSG_ERROR([Required header not found <sys/types.h>])], [])
AC_CHECK_HEADERS([sys/time.h], [], [AC_MSG_ERROR([Required header not found <sys/time.h>])], [])
AC_CHECK_HEADERS([sys/select.h], [], [AC_MSG_ERROR([Required header not found <sys/select.h>])], [])
AC_CHECK_HEADERS([sys/socket.h], [], [AC_MSG_ERROR([Required header not found <sys/socket.h>])], [])
AC_CHECK_HEADERS([sys/ioctl.h], [], [AC_MSG_ERROR([Required header not found <sys/ioctl.h>])], [])
AC_CHECK_HEADERS([sys/uio.h], [], [AC_MSG_ERROR([Required header not found <sys/uio.h>])], [])
AC_CHECK_HEADERS([sys/utsname.h], [], [AC_MSG_ERROR([Required header not found <sys/utsname.h>])], [])
AC_CHECK_HEADERS([sys/statvfs.h], [], [AC_MSG_ERROR([Required header not found <sys/statvfs.h>])], [])
AC_CHECK_HEADERS([sys/socket.h], [], [AC_MSG_ERROR([Required header not found <sys/socket.h>])], [])
AC_CHECK_HEADERS([sys/poll.h], [], [AC_MSG_ERROR([Required header not found <sys/poll.h>])], [])
AC_CHECK_HEADERS([sys/un.h], [], [AC_MSG_ERROR([Required header not found <sys/un.h>])], [])
AC_CHECK_HEADERS([sys/stat.h], [], [AC_MSG_ERROR([Required header not found <sys/stat.h>])], [])
AC_CHECK_HEADERS([sys/param.h], [], [AC_MSG_ERROR([Required header not found <sys/param.h>])], [])
AC_CHECK_HEADERS([sys/resource.h], [], [AC_MSG_ERROR([Required header not found <sys/resource.h>])], [])
AC_CHECK_HEADERS([sys/capability.h], [], [AC_MSG_ERROR([Required header not found <sys/capability.h>])], [])
AC_CHECK_HEADERS([sys/prctl.h], [], [AC_MSG_ERROR([Required header not found <sys/prctl.h>])], [])
AC_CHECK_HEADERS([sys/wait.h], [], [AC_MSG_ERROR([Required header not found <sys/wait.h>])], [])
AC_CHECK_HEADERS([linux/kd.h], [], [AC_MSG_ERROR([Required header not found <linux/kd.h>])], [])
AC_CHECK_HEADERS([linux/input.h], [], [AC_MSG_ERROR([Required header not found <linux/input.h>])], [])
AC_CHECK_HEADERS([linux/tcp.h], [], [AC_MSG_ERROR([Required header not found <linux/tcp.h>])], [])
AC_CHECK_HEADERS([arpa/inet.h], [], [AC_MSG_ERROR([Required header not found <arpa/inet.h>])], [])
AC_CHECK_HEADERS([arpa/tftp.h], [], [AC_MSG_ERROR([Required header not found <arpa/tftp.h>])], [])
AC_CHECK_HEADERS([netinet/in.h], [], [AC_MSG_ERROR([Required header not found <netinet/in.h>])], [])
AC_CHECK_HEADERS([netinet/tcp.h], [], [AC_MSG_ERROR([Required header not found <netinet/tcp.h>])], [])
AC_CHECK_HEADERS([net/if.h], [], [AC_MSG_ERROR([Required header not found <net/if.h>])], [])
AC_CHECK_HEADERS([ifaddrs.h], [], [AC_MSG_ERROR([Required header not found <ifaddrs.h>])], [])
AC_CHECK_HEADERS([netdb.h], [], [AC_MSG_ERROR([Required header not found <netdb.h>])], [])
AC_CHECK_HEADERS([poll.h], [], [AC_MSG_ERROR([Required header not found <poll.h>])], [])
AC_CHECK_HEADERS([pthread.h], [], [AC_MSG_ERROR([Required header not found <pthread.h>])], [])
AC_CHECK_HEADERS([limits.h], [], [AC_MSG_ERROR([Required header not found <limits.h>])], [])
AC_CHECK_HEADERS([signal.h], [], [AC_MSG_ERROR([Required header not found <signal.h>])], [])
AC_CHECK_HEADERS([termios.h], [], [AC_MSG_ERROR([Required header not found <termios.h>])], [])
AC_CHECK_HEADERS([termio.h], [], [AC_MSG_ERROR([Required header not found <termio.h>])], [])
AC_CHECK_HEADERS([sgtty.h], [], [AC_MSG_ERROR([Required header not found <sgtty.h>])], [])
AC_CHECK_HEADERS([fcntl.h], [], [AC_MSG_ERROR([Required header not found <fcntl.h>])], [])
AC_CHECK_HEADERS([malloc.h], [], [AC_MSG_ERROR([Required header not found <malloc.h>])], [])
AC_CHECK_HEADERS([alloca.h], [], [AC_MSG_ERROR([Required header not found <alloca.h>])], [])
AC_CHECK_HEADERS([time.h], [], [AC_MSG_ERROR([Required header not found <time.h>])], [])
AC_CHECK_HEADERS([pwd.h], [], [AC_MSG_ERROR([Required header not found <pwd.h>])], [])
AC_CHECK_HEADERS([utime.h], [], [AC_MSG_ERROR([Required header not found <utime.h>])], [])
AC_CHECK_HEADERS([libgen.h], [], [AC_MSG_ERROR([Required header not found <libgen.h>])], [])
AC_CHECK_HEADERS([locale.h], [], [AC_MSG_ERROR([Required header not found <locale.h>])], [])
AC_CHECK_HEADERS([errno.h], [], [AC_MSG_ERROR([Required header not found <errno.h>])], [])
AC_CHECK_HEADERS([stdbool.h], [], [AC_MSG_ERROR([Required header not found <stdbool.h>])], [])
AC_CHECK_HEADERS([setjmp.h], [], [AC_MSG_ERROR([Required header not found <setjmp.h>])], [])
AC_CHECK_HEADERS([png.h], [], [AC_MSG_ERROR([Required header not found <png.h>])], [])
AC_CHECK_HEADERS([jpeglib.h], [], [AC_MSG_ERROR([Required header not found <jpeglib.h>])], [])
AC_CHECK_HEADERS([FLAC/metadata.h], [], [AC_MSG_ERROR([Required header not found <FLAC/metadata.h>])], [])
AC_CHECK_HEADERS([libexif/exif-data.h], [], [AC_MSG_ERROR([Required header not found <libexif/exif-data.h.h>])], [])
AC_CHECK_HEADERS([openssl/ssl.h], [], [AC_MSG_ERROR([Required header not found <openssl/ssl.h.h>])], [])
AC_CHECK_HEADERS([openssl/evp.h], [], [AC_MSG_ERROR([Required header not found <openssl/evp.h>])], [])
AC_CHECK_HEADERS([openssl/err.h], [], [AC_MSG_ERROR([Required header not found <openssl/err.h.h>])], [])
AC_CHECK_HEADERS([openssl/ec.h], [], [AC_MSG_ERROR([Required header not found <openssl/ec.h.h>])], [])
AC_CHECK_HEADERS([libudev.h], [], [AC_MSG_ERROR([Required header not found <libudev.h>])], [])

# Files to generate via autotools (.am or .in source files)
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/inc/Makefile])
AC_CONFIG_FILES([src/app/Makefile])
AC_CONFIG_FILES([src/inc/alac/Makefile])
AC_CONFIG_FILES([src/inc/libfaad/Makefile])
AC_CONFIG_FILES([src/inc/cie/Makefile])
AC_CONFIG_FILES([src/inc/fft/Makefile])
AC_CONFIG_FILES([src/inc/id3v2/Makefile])
AC_CONFIG_FILES([src/inc/microhttpd/Makefile])
AC_CONFIG_FILES([src/inc/sha1/Makefile])
AC_CONFIG_FILES([src/inc/sqlite3/Makefile])
AC_CONFIG_FILES([src/inc/zlib/Makefile])

# Define SVN version in configuration header
AC_DEFINE(APP_VER, "SVN_VER", [SVN Revision])

# Create configuration header file
AC_CONFIG_HEADERS([autoconf.h])

# Generate the final Makefiles
AC_OUTPUT
