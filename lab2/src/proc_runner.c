#include "proc_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

proc_handle_t proc_run_async(const char* command) {
    if (!command) return INVALID_PROC_HANDLE;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char* cmd_copy = _strdup(command);
    if (!cmd_copy) return INVALID_PROC_HANDLE;

    BOOL success = CreateProcessA(
        NULL, cmd_copy, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi
    );
    free(cmd_copy);

    if (!success) {
        return INVALID_PROC_HANDLE;
    }

    CloseHandle(pi.hThread);
    return pi.hProcess;
}

int proc_wait(proc_handle_t handle, int* exit_code) {
    if (handle == INVALID_PROC_HANDLE || !exit_code) return -1;

    WaitForSingleObject(handle, INFINITE);

    DWORD code = 0;
    GetExitCodeProcess(handle, &code);
    *exit_code = (int)code;

    CloseHandle(handle);
    return 0;
}

#else // LINUX / POSIX

#include <unistd.h>
#include <sys/wait.h>

proc_handle_t proc_run_async(const char* command) {
    if (!command) return INVALID_PROC_HANDLE;

    pid_t pid = fork();

    if (pid < 0) {
        return INVALID_PROC_HANDLE;
    } else if (pid == 0) {
        execl("/bin/sh", "sh", "-c", command, (char*)NULL);
        exit(127);
    }

    return pid;
}

int proc_wait(proc_handle_t handle, int* exit_code) {
    if (handle == INVALID_PROC_HANDLE || !exit_code) return -1;

    int status = 0;
    if (waitpid(handle, &status, 0) == -1) {
        return -1;
    }

    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
    } else {
        *exit_code = -1;
    }

    return 0;
}

#endif