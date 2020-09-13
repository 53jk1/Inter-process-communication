#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "calc.h"

// Re-declaration of two functions, in case MinGW Platform SDK does not have them in a given version.

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365440.aspx
BOOL WINAPI GetNamedPipeClientProcessId(
    HANDLE Pipe,
    PULONG ClientProcessId
);

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365442.aspx
BOOL WINAPI GetNamedPipeClientSessionId(
    HANDLE Pipe,
    PULONG ClientSessionId
);

HANDLE WaitForNewClient(void) {
    // Create a new pipeline (bidirectional access, packet mode).
    HANDLE h = CreateNamedPipe(
        CALCSERVER_PIPE,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        sizeof(calc_response),
        sizeof(calc_request) + sizeof(unsigned int) * CALC_REQUEST_MAX_DATA,
        0,
        NULL);

    if (h == INVALID_HANDLE_VALUE) {
        printf("error: failed to create pipe (%u)\n", (unsigned int) GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    // Wait for a connection from the other side of the pipe.
    BOOL result = ConnectNamedPipe(h, NULL);
    if (result == TRUE) {
        return h;
    }

    // In the above case, a race condition occurs, ie the client can connect to the pipe between CreateNamedPipe and ConnectNamedPipe. In this case, GetLastError will return ERROR_PIPE_CONNECTED.
    if (GetLastError() == ERROR_PIPE_CONNECTED) {
        return h;
    }

    printf("error: failed to connect pipe (%u)\n", (unsigned int) GetLastError());
    CloseHandle(h)
    return INVALID_HANDLE_VALUE;
}

calc_request *GetCalcRequest(HANDLE h, size_t *item_count) {
    // Block execution until data is received.
    DWORD bytes_read;
    BOOL result = ReadFile(h, NULL, 0, &bytes_read, NULL);
    if (!result) {
        printf("error: failed on initial read (%u)\n", (unsigned int)GetLastError());
        return NULL;
    }

    // Check the amount of data available.
    DWORD bytes_avail;
    result = PeekNamedPipe(h, NULL, 0, NULL, &bytes_avail, NULL);

    // Sample package-related error detection.
    if (bytes_avail < sizeof(calc_request)) {
        printf("protocol error (packet too small)\n");
        return NULL;
    }

    if ((bytes_avail - sizeof(calc_request)) % sizeof(unsigned int) != 0) {
        printf("protocol error (data misaligned)\n");
        return NULL;
    }

    // Allocate memory to structure then receive message.
    calc_request *creq = malloc(bytes_avail);
    ReadFile(h, creq, bytes_avail, &bytes_read, NULL);

    if (creq->op != OP_SUM && creq->op != OP_MUL && creq->op != OP_DIV) {
        printf("protocol error (invalid operation)\n");
        free(creq);
        return NULL;
    }

    *item_count = (bytes_avail - sizeof(calc_request)) / sizeof(unsigned int);
    return creq;
}

int Calc(unsigned int * result, calc_request *cr, size_t count) {
    switch(cr->op) {
        case OP_SUM:
            *result = 0;
            for (size_t i = 0; i < count; i++) {*result += cr->data[i];}
            break;

        case OP_MUL:
            *result = 1;
            for (size_t i = 0; i < count; i++) {*result *= cr->data[i];}
            break;

        case OP_DIV:
            if (count == 0) {
                return -1;
            }
            *result = cr->data[0];
            for (size_t i = 1; i < count; i++) {
                if (cr->data[i] == 0) {
                    return -1;
                }
                *result /= cr->data[i];
            }
            break;
        
        default:
            return -1;
    }

    return 0;
}

int SendCalcResponse(HANDLE h, calc_response *cresp) {
    DWORD bytes_sent;
    if (!WriteFile(h, cresp, sizeof(*cresp), &bytes_sent, NULL)) {
        printf("error: failed to send data over pipe (%u)\n", (unsigned int)GetLastError());
        return -1;
    }

    if (bytes_sent != sizeof(*cresp)) {
        printf("error: failed to send all of the data over pipe (%u, %u)\n", (unsigned int)bytes_sent, (unsigned int)GetLastError());
        return -1;
    }

    return 0;
}

int main(void) {
    // Single-threaded pseudo-calculator server serving one client at a time.

    for(;;) {
        // Create a pipe and wait for connection.
        HANDLE h = WaitForNewClient();
        if (h == INVALID_HANDLE_VALUE) {
            continue;
        }

        // List customer information.
        ULONG pid, sid;
        GetNamedPipeClientProcessId(h, &pid);
        GetNamedPipeClientSessionId(h, &sid);
        printf("info: client connected! (pid=%u, sid=%u)\n", (unsigned int)pid, (unsigned int)sid);

        // Receive the request.
        size_t item_count = 0;
        calc_request *creq = GetCalcRequest(h, &item_count);
        if (creq == NULL) {
            CloseHandle(h);
            continue;
        }

        // Calculate the result.
        unsigned int value = 0;
        calc_response cresp;
        memset(&cresp, 0, sizeof(cresp));
        if (Calc(&value, creq, item_count) == 0) {
            cresp.res = RES_OK;
            cresp.value = value;
        } else {
            cresp.res = RES_ERROR;
        }
        printf("info: result for the client is %u\n", value);
        free(creq);

        // Send the result back and disconnect the client.
        SendCalcResponse(h, &cresp);
        DisconnectNamedPipe(h);
        CloseHandle(h);
    }

    return 0;
}