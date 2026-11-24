#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "platform_os.h"

static const char* LOG_FILE_PATH = "lab3.log";
static volatile bool g_running = true;

void write_log(const char* message) {
    ipc_lock();
    FILE* f = fopen(LOG_FILE_PATH, "a");
    if (f) {
        char time_buf[64];
        get_current_timestamp(time_buf, sizeof(time_buf));
        fprintf(f, "[%s] [PID: %d] %s\n", time_buf, get_current_pid(), message);
        fclose(f);
    }
    ipc_unlock();
}

void run_copy_1(void) {
    write_log("Copy 1 started.");
    
    ipc_lock();
    int64_t* counter = ipc_get_counter_ptr();
    *counter += 10;
    ipc_unlock();

    write_log("Copy 1 finished (added 10 to counter).");
}

void run_copy_2(void) {
    write_log("Copy 2 started.");

    ipc_lock();
    int64_t* counter = ipc_get_counter_ptr();
    *counter *= 2;
    ipc_unlock();

    os_sleep_ms(2000);

    ipc_lock();
    *counter /= 2;
    ipc_unlock();

    write_log("Copy 2 finished (multiplied and divided counter by 2).");
}

void* timer_300ms_thread_func(void* arg) {
    (void)arg;
    int64_t* counter = ipc_get_counter_ptr();
    while (g_running) {
        os_sleep_ms(300);
        ipc_lock();
        (*counter)++;
        ipc_unlock();
    }
    return NULL;
}

void* cli_thread_func(void* arg) {
    (void)arg;
    int64_t new_val = 0;
    int64_t* counter = ipc_get_counter_ptr();
    while (g_running) {
        if (scanf("%lld", (long long*)&new_val) == 1) {
            ipc_lock();
            *counter = new_val;
            ipc_unlock();
            printf(">>> Counter set to: %lld\n", (long long)new_val);
        } else {
            break;
        }
    }
    return NULL;
}

void* master_thread_func(void* arg) {
    (void)arg;
    char exe_path[1024];
    get_app_path(exe_path, sizeof(exe_path));

    proc_handle_t child1 = INVALID_PROC_ID;
    proc_handle_t child2 = INVALID_PROC_ID;
    int elapsed_ms = 0;
    int64_t* counter = ipc_get_counter_ptr();

    while (g_running) {
        os_sleep_ms(100);
        elapsed_ms += 100;

        if (elapsed_ms % 1000 == 0) {
            ipc_lock();
            int64_t current_val = *counter;
            ipc_unlock();

            char msg[128];
            snprintf(msg, sizeof(msg), "Counter value: %lld", (long long)current_val);
            write_log(msg);
        }

        if (elapsed_ms % 3000 == 0) {
            if (is_process_running(child1)) {
                write_log("Warning: Copy 1 is still running, skipping launch.");
            } else {
                close_proc_handle(child1);
                child1 = spawn_child(exe_path, "--copy1");
            }

            if (is_process_running(child2)) {
                write_log("Warning: Copy 2 is still running, skipping launch.");
            } else {
                close_proc_handle(child2);
                child2 = spawn_child(exe_path, "--copy2");
            }
        }
    }

    close_proc_handle(child1);
    close_proc_handle(child2);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (!ipc_init()) {
        fprintf(stderr, "Failed to initialize IPC.\n");
        return 1;
    }

    if (argc > 1) {
        if (strcmp(argv[1], "--copy1") == 0) {
            run_copy_1();
            ipc_cleanup();
            return 0;
        } else if (strcmp(argv[1], "--copy2") == 0) {
            run_copy_2();
            ipc_cleanup();
            return 0;
        }
    }

    write_log("Process instance started.");

    bool is_master = ipc_try_acquire_master();
    printf("Process PID: %d [%s]\n", get_current_pid(), is_master ? "MASTER" : "SLAVE");
    printf("Type a number and press Enter to change counter value.\n\n");

    thread_handle_t t_300ms = os_thread_create(timer_300ms_thread_func, NULL);
    thread_handle_t t_cli = os_thread_create(cli_thread_func, NULL);
    thread_handle_t t_master = 0;

    if (is_master) {
        t_master = os_thread_create(master_thread_func, NULL);
    }

    os_thread_join(t_cli);

    g_running = false;
    os_thread_join(t_300ms);
    if (is_master) {
        os_thread_join(t_master);
    }

    ipc_cleanup();
    return 0;
}