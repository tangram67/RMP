/*
 * Derived from addr2line.c and associated binutils files, version 2.18.
 */

#ifndef ADDR2LINE_H_
#define ADDR2LINE_H_

#ifdef __cplusplus
extern "C" {
#endif

int libtrace_init(const char *file_name, const char *section_name, const char *target);
int libtrace_demangle(void *addr, char *buf_func, size_t buf_func_len, char *buf_file, size_t buf_file_len, ...);
void libtrace_close(void);

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif
