#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#define OK 0

#define CREATE_MUTEX_ERROR 1
#define CREATE_EVENT_ERROR 2
#define CREATE_READER_THREAD_ERROR 3
#define CREATE_WRITER_THREAD_ERROR 3

#define READERS_COUNT 3
#define WRITERS_COUNT 3

#define COUNT 3

const DWORD sleep_time_writer = 50;
const DWORD sleep_time_reader = 30;

volatile LONG waiting_writers = 0;
volatile LONG waiting_readers = 0;
volatile LONG active_readers = 0;

HANDLE can_read, can_write, mutex;
HANDLE reader_threads[READERS_COUNT];
HANDLE writer_threads[WRITERS_COUNT];

char data = 'A' - 1;
bool flag = false;

bool turn(HANDLE event) {
    return WaitForSingleObject(event, 0) == WAIT_OBJECT_0;
}

void start_read() {
    InterlockedIncrement(&waiting_readers);
    if (turn(can_write) || flag) {
        WaitForSingleObject(can_read, INFINITE);
    }
    WaitForSingleObject(mutex, INFINITE);
    InterlockedDecrement(&waiting_readers);
    InterlockedIncrement(&active_readers);
    SetEvent(can_read);
    ReleaseMutex(mutex);
}


void stop_read() {
    InterlockedDecrement(&active_readers);
    if (active_readers == 0) {
        SetEvent(can_write);
    }
}


DWORD WINAPI reader(CONST LPVOID param) {
    for (;data < 'A' - 1 + WRITERS_COUNT * COUNT;) {
        start_read();
        printf("Reader %ld <<<<< %c\n", GetCurrentThreadId(), data);
        stop_read();
        Sleep(sleep_time_reader);
    }
    return 0;
}

void start_write() {
    InterlockedIncrement(&waiting_writers);
    if (active_readers > 0 || flag) {
        WaitForSingleObject(can_write, INFINITE);
    }
    InterlockedDecrement(&waiting_writers);
    flag = true;
    ResetEvent(can_write);
}


void stop_write(){
    flag = false;
    if (waiting_readers) {
        SetEvent(can_read);
        return;
    }
    SetEvent(can_write);
}


DWORD WINAPI writer(CONST LPVOID param) {
    for (int i = 0; i < COUNT; ++i) {
        start_write();
        printf("Writer %ld >>>>> %c\n", GetCurrentThreadId(), ++data);
        stop_write();
        Sleep(sleep_time_writer);
    }
    return 0;
}

int init_handles() {
    mutex = CreateMutex(NULL, FALSE, NULL);
    if (mutex == NULL) {
        perror("CreateMutex");
        return CREATE_MUTEX_ERROR;
    }

    can_read = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (can_read == NULL) {
        perror("CreateEvent (canRead)");
        return CREATE_EVENT_ERROR;
    }

    can_write = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (can_write == NULL) {
        perror("CreateEvent (canWrite)");
        return CREATE_EVENT_ERROR;
    }

    return OK;
}

int create_threads() {
    for (int i = 0; i < WRITERS_COUNT; i++) {
        writer_threads[i] = CreateThread(NULL, 0, &writer, NULL, 0, NULL);
        if (writer_threads[i] == NULL) {
            perror("CreateThread (writer)");
            return CREATE_WRITER_THREAD_ERROR;
        }
    }
    
    for (int i = 0; i < READERS_COUNT; ++i) {
        reader_threads[i] = CreateThread(NULL, 0, &reader, NULL, 0, NULL);
        if (reader_threads[i] == NULL) {
            perror("CreateThread (reader)");
            return CREATE_READER_THREAD_ERROR;
        }
    }

    return OK;
}

void close() {
    for (int i = 0; i < READERS_COUNT; ++i) {
        CloseHandle(reader_threads[i]);
    }

    for (int i = 0; i < WRITERS_COUNT; ++i) {
        CloseHandle(writer_threads[i]);
    }

    CloseHandle(can_read);
    CloseHandle(can_write);
    CloseHandle(mutex);
}

int main(void)
{
    //setbuf(stdout, NULL);
    //srand(time(NULL));

    int check = init_handles();
    if (check) {
        return check;
    }
        
    check = create_threads();
    if (check) {
        return check;
    }

    WaitForMultipleObjects(READERS_COUNT, reader_threads, TRUE, INFINITE);
    WaitForMultipleObjects(WRITERS_COUNT, writer_threads, TRUE, INFINITE);

    close();
    return OK;
}