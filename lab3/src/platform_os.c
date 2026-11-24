#include "platform_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

static HANDLE g_hMapFile = NULL;
static int64_t* g_pSharedCounter = NULL;
static HANDLE g_hMutex = NULL;
static HANDLE g_hMasterMutex = NULL;

bool ipc_init(void) {
    g_hMutex = CreateMutexA(NULL, FALSE, "Local\\OS_Lab3_Data_Mutex");
    if (!g_hMutex) return false;

    g_hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int64_t), "Local\\OS_Lab3_SHM"
    );
    if (!g_hMapFile) return false;

    g_pSharedCounter = (int64_t*)MapViewOfFile(g_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int64_t));
    return g_pSharedCounter != NULL;
}

void ipc_cleanup(void) {
    if (g_pSharedCounter) UnmapViewOfFile(g_pSharedCounter);
    if (g_hMapFile) CloseHandle(g_hMapFile);
    if (g_hMutex) CloseHandle(g_hMutex);
    if (g_hMasterMutex) CloseHandle(g_hMasterMutex);
}

void ipc_lock(void) {
    WaitForSingleObject(g_hMutex, INFINITE);
}

void ipc_unlock(void) {
    ReleaseMutex(g_hMutex);
}

int64_t* ipc_get_counter_ptr(void) {
    return g_pSharedCounter;
}

bool ipc_try_acquire_master(void) {
    g_hMasterMutex = CreateMutexA(NULL, FALSE, "Local\\OS_Lab3_Master_Mutex");
    if (!g_hMasterMutex) return false;
    return (WaitForSingleObject(g_hMasterMutex, 0) == WAIT_OBJECT_0);
}

int get_current_pid(void) {
    return (int)GetCurrentProcessId();
}

void get_current_timestamp(char* buffer, size_t size) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void get_app_path(char* buffer, size_t size) {
    GetModuleFileNameA(NULL, buffer, (DWORD)size);
}

thread_handle_t os_thread_create(thread_func_t func, void* arg) {
    return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
}

void os_thread_join(thread_handle_t thread) {
    if (thread != NULL) {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

void os_sleep_ms(int ms) {
    Sleep((DWORD)ms);
}

proc_handle_t spawn_child(const char* app_path, const char* arg) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "\"%s\" %s", app_path, arg);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return INVALID_PROC_ID;
    }
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

bool is_process_running(proc_handle_t handle) {
    if (handle == INVALID_PROC_ID) return false;
    DWORD exitCode;
    if (GetExitCodeProcess(handle, &exitCode)) {
        return (exitCode == STILL_ACTIVE);
    }
    return false;
}

void close_proc_handle(proc_handle_t handle) {
    if (handle != INVALID_PROC_ID) {
        CloseHandle(handle);
    }
}

#else // LINUX / POSIX

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

static int g_shm_fd = -1;
static int64_t* g_pSharedCounter = NULL;
static sem_t* g_sem = SEM_FAILED;
static int g_master_lock_fd = -1;

bool ipc_init(void) {
    g_sem = sem_open("/os_lab3_sem", O_CREAT, 0666, 1);
    if (g_sem == SEM_FAILED) return false;

    g_shm_fd = shm_open("/os_lab3_shm", O_CREAT | O_RDWR, 0666);
    if (g_shm_fd == -1) return false;

    ftruncate(g_shm_fd, sizeof(int64_t));
    g_pSharedCounter = (int64_t*)mmap(NULL, sizeof(int64_t), PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
    return (g_pSharedCounter != MAP_FAILED);
}

void ipc_cleanup(void) {
    if (g_pSharedCounter && g_pSharedCounter != MAP_FAILED) {
        munmap(g_pSharedCounter, sizeof(int64_t));
    }
    if (g_shm_fd != -1) close(g_shm_fd);
    if (g_sem != SEM_FAILED) sem_close(g_sem);
    if (g_master_lock_fd != -1) close(g_master_lock_fd);
}

void ipc_lock(void) {
    sem_wait(g_sem);
}

void ipc_unlock(void) {
    sem_post(g_sem);
}

int64_t* ipc_get_counter_ptr(void) {
    return g_pSharedCounter;
}

bool ipc_try_acquire_master(void) {
    g_master_lock_fd = open("/tmp/os_lab3_master.lock", O_CREAT | O_RDWR, 0666);
    if (g_master_lock_fd < 0) return false;
    return (flock(g_master_lock_fd, LOCK_EX | LOCK_NB) == 0);
}

int get_current_pid(void) {
    return (int)getpid();
}

void get_current_timestamp(char* buffer, size_t size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm_buf;
    localtime_r(&tv.tv_sec, &tm_buf);
    
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);
    snprintf(buffer, size, "%s.%03d", time_str, (int)(tv.tv_usec / 1000));
}

void get_app_path(char* buffer, size_t size) {
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len != -1) {
        buffer[len] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

thread_handle_t os_thread_create(thread_func_t func, void* arg) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, func, arg) != 0) {
        return (pthread_t)0;
    }
    return thread;
}

void os_thread_join(thread_handle_t thread) {
    if (thread != 0) {
        pthread_join(thread, NULL);
    }
}

void os_sleep_ms(int ms) {
    usleep(ms * 1000);
}

proc_handle_t spawn_child(const char* app_path, const char* arg) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(app_path, app_path, arg, (char*)NULL);
        _exit(127);
    }
    return pid;
}

bool is_process_running(proc_handle_t handle) {
    if (handle <= 0) return false;
    int status;
    pid_t res = waitpid(handle, &status, WNOHANG);
    return (res == 0);
}

void close_proc_handle(proc_handle_t handle) {
    (void)handle;
}

#endif