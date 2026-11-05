#include <stdio.h>
#include <stdlib.h>
#include "proc_runner.h"

int main(int argc, char* argv[]) {
    const char* cmd;

    if (argc > 1) {
        cmd = argv[1];
    } else {
#if defined(_WIN32)
        cmd = "cmd.exe /c timeout /t 2 >nul & exit 42";
#else
        cmd = "sleep 2; exit 42";
#endif
    }

    printf("[Parent] Starting background process: '%s'\n", cmd);

    proc_handle_t proc = proc_run_async(cmd);
    if (proc == INVALID_PROC_HANDLE) {
        fprintf(stderr, "[Parent] Failed to launch process!\n");
        return 1;
    }

    printf("[Parent] Process is running in background! Parent is doing some work...\n");
    for (int i = 1; i <= 3; i++) {
        printf("[Parent] Doing work step %d...\n", i);
    }

    printf("[Parent] Now waiting for background process to finish...\n");
    int exit_code = 0;
    if (proc_wait(proc, &exit_code) == 0) {
        printf("[Parent] SUCCESS! Process finished with exit code: %d\n", exit_code);
    } else {
        fprintf(stderr, "[Parent] Error waiting for process.\n");
        return 1;
    }

    return 0;
}