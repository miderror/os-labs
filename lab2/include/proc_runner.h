#ifndef PROC_RUNNER_H
#define PROC_RUNNER_H

#if defined(_WIN32)
    #include <windows.h>
    typedef HANDLE proc_handle_t;
    #define INVALID_PROC_HANDLE INVALID_HANDLE_VALUE
#else
    #include <sys/types.h>
    typedef pid_t proc_handle_t;
    #define INVALID_PROC_HANDLE ((pid_t)-1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

proc_handle_t proc_run_async(const char* command);

int proc_wait(proc_handle_t handle, int* exit_code);

#ifdef __cplusplus
}
#endif

#endif