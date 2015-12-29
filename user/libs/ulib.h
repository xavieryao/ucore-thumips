#ifndef __USER_LIBS_ULIB_H__
#define __USER_LIBS_ULIB_H__

#include <defs.h>

void __warn(const char *file, int line, const char *fmt, ...);
void __noreturn __panic(const char *file, int line, const char *fmt, ...);

#define warn(...)                                       \
    __warn(__FILE__, __LINE__, __VA_ARGS__)

#define panic(...)                                      \
    __panic(__FILE__, __LINE__, __VA_ARGS__)

#define assert(x)                                       \
    do {                                                \
        if (!(x)) {                                     \
            panic("assertion failed: %s", #x);          \
        }                                               \
    } while (0)

// static_assert(x) will generate a compile-time error if 'x' is false.
#define static_assert(x)                                \
    switch (x) { case 0: case (x): ; }
    
typedef unsigned int time_t;

int fprintf(int fd, const char *fmt, ...);

void __noreturn exit(int error_code);
int fork(void);
int wait(void);
int waitpid(int pid, int *store);
void yield(void);
int kill(int pid);
int getpid(void);
void print_pgdir(void);
int sleep(unsigned int time);
time_t gettime_msec(void);
int __exec(const char *name, const char **argv);

#define __exec0(name, path, ...)                \
({ const char *argv[] = {path, ##__VA_ARGS__, NULL}; __exec(name, argv); })

#define exec(path, ...)                         __exec0(NULL, path, ##__VA_ARGS__)
#define nexec(name, path, ...)                  __exec0(name, path, ##__VA_ARGS__)

void lab6_set_priority(uint32_t priority); //only for lab6

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

typedef unsigned int size_t;
typedef int ssize_t;

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#endif /* !__USER_LIBS_ULIB_H__ */

