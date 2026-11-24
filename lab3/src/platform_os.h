#ifndef PLATFORM_OS_H
#define PLATFORM_OS_H

#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
    #include <windows.h>
    typedef HANDLE proc_handle_t;
    typedef HANDLE thread_handle_t;
    #define INVALID_PROC_ID NULL
#else
    #include <sys/types.h>
    #include <pthread.h>
    typedef pid_t proc_handle_t;
    typedef pthread_t thread_handle_t;
    #define INVALID_PROC_ID ((pid_t)-1)
#endif

typedef void* (*thread_func_t)(void* arg);

bool ipc_init(void);
void ipc_cleanup(void);

void ipc_lock(void);
void ipc_unlock(void);

int64_t* ipc_get_counter_ptr(void);

bool ipc_try_acquire_master(void);

int get_current_pid(void);
void get_current_timestamp(char* buffer, size_t size);
void get_app_path(char* buffer, size_t size);

thread_handle_t os_thread_create(thread_func_t func, void* arg);
void os_thread_join(thread_handle_t thread);
void os_sleep_ms(int ms);

proc_handle_t spawn_child(const char* app_path, const char* arg);
bool is_process_running(proc_handle_t handle);
void close_proc_handle(proc_handle_t handle);

#endif