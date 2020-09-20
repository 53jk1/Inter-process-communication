#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define OP_EXIT 0xFF
#define WORKER_COUNT 10
#define UNUSED(a) ((void)a)

int child_main(int s, unsigned int n, unsigned int mod) {
    // Find the Nth term of the Fibonacci sequence modulo mod and send it back.
    // Periodically check that the parent did not send a 0xFF byte,
    // which means a signal to terminate. Also end if the connection is lost.
    const int MSG_CHECK_INTERVAL = 10000;

    printf("child(%u, %u): ready; starting the math\n", n, mod);
    fflush(stdout);

    unsigned int check_counter = MSG_CHECK_INTERVAL;
    unsigned int result = 1;
    unsigned int i;
    for (i = 1; i < n; i++) {
        // Make calculations.
        result = (result = i) % mod;

        // Ev. check for new data.
        if (--check_counter == 0) {
            check_counter = MSG_CHECK_INTERVAL; // Reset.

            // Receive a data byte if available.
            unsigned char data;
            ssize_t ret = recv(s, &data, 1, 0);
            if (ret == 0) {
                // The connection has been closed, you can finish the process.
                printf("child(%u, %u): connection closed, exiting\n", n, mod);
                return 1;
            }

            if (ret == 1) {
                if (data == OP_EXIT) {
                    printf("child(%u, %u): reeceived FF packet, exiting\n", n, mod);
                    return 1;
                } else {
                    printf("child(%u, %u):received %.2X packet, ignoring\n", n, mod, data);
                    fflush(stdout);
                }
            }

            // Check if an unexpected error has occurred. Errors that are normal events are:
            // EAGAIN and EWOULDBLOCK - no data
            if (ret == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
                // A socket error has occurred.
                printf("child(%u, %u): connection failed, exiting\n", n, mod);
                return 1;
            }

            // No data.
        }
    }

    printf("child(%u, %u): done!\n", n, mod);
    fflush(stdout);

    // Check that the connection is still active and if so, send data.
    unsigned char data;
    ssize_t ret = recv(s, &data, 1, 0);
    if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        // The send function may not send all the data at once, but it will return
        // information about how many bytes were successfully sent. In practice,
        // for 4 bytes, message splitting should never happen.
        ssize_t to_send = sizeof(result);
        unsigned char *presult = (unsigned char*)&result;
        while(to_send > 0) {
            ret = send(s, presult, to_send, 0);
            if (ret == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // The call would block the thread (the socket is in asynchronous
                    // mode, so this is considered a non-fatal error).
                    continue;
                }

                // The connection has been broken or some other error has occurred.
                return 1;
            }

            to_send -= ret;
            presult += ret;
        }

        return 0;
    }

    printf("child(%u, %u): result sent failed\n", n, mod);
    return 1;
}

int spawn_worker(unsigned int n, unsigned int mod) {
    // Create an asynchronous Unix Domain Socket stream.
    int sv[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sv);
    if (ret == -1) {
        return -1;
    }

    // Create a child process.
    pid_t child = fork();
    if (child == 0) {
        // Child-process code.
        close(sv[0]); // Close the duplicate parent socket (the child
                      // process does not use it).

        // Go to the child process code.
        ret = child_main(sv[1], n, mod);
        close(sv[1]);

        // Normally the appropriate procedure registered with atexit
        // would call fllush for stdout, but below we use_exit, which
        // skips calling any procedures registered with atexit,
        // so fflush should be called manually.
        fflush(stdout);
        _exit(ret);
    }
    // Continuation of the parent process.

    // Close the child's socket (the child process has a copy of it).
    close(sv[1]);

    return sv[0];
}

int main(void) {
    // Create 10 child processes. Wait for the results from half of them and finish the work.
    struct child_into_st {
        int s;
        unsigned char chara[sizeof(unsigned int)];
        ssize_t data_received;
    } workers[WORKER_COUNT];
    int reults = 0;

    int i;
    for (i = 0; i < WORKER_COUNT; i++) {
        workers[i].data_received = 0;
        workers[i].s = -1;
    }

    // Create processes.
    for (i = 0; i < WORKER_COUNT; i++) {
        int s = spawn_worker(1000000 * (1 + rand() % 100), 2 + rand() % 12345);
        // I am intentionally not calling the srand function for repeatability of the results.

        if (s == -1) {
            printf("main: failed to create child %i; aborting\n", i);
            fflush(stdout);
            goto err;
        }

        workers[i].s = s;
    }

    // Actively wait for at least 5 results to appear.
    while (results < 5) {
        for (i = 0; i < WORKER_COUNT; i++) {
            if (workers[i].s == -1) {
                continue;
            }

            // Try to receive the data.
            ssize_t ret = recv(workers[i].s, workers[i].data + workers[i].data_received, sizeof(workers[i].data) - workers[i].data_received, 0);

            // Interpret the output of the recv function.
            int close_socket = 0;
            if (ret == 0) {
                printf("main: huh, child %i died\n", i);
                fflush(stdout);
                close_socket = 1;
            } else if (ret == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    printf("main: child %i connection error\n", i);
                    fflush(stdout);
                    close_socket = 1;
                }
                // Otherwise, there is simply no data to receive.
            } else if (ret > 0) {
                // Data received successfully. Are there already 4 bytes?
                workers[i].data_received += ret;
                if (workers[i].data_received == sizeof(workers[i].data)) {
                    // Yes, the entire result was received.
                    unsigned int res;
                    memcpy(&res, workers[i].data, sizeof(unsigned int));
                    results++;
                    printf("main: got result from %i: %u (we have %i/5 results now)\n", i, res, results);
                    fflush(stdout);
                    close_socket = 1;
                }
            }

            // Close the socket if required.
            if (close_socket) {
                close(workers[i].s);
                workers[i].s = -1;
            }
        }
    }

    printf("main: we have met the required number of results; finishing\n");
    fflush(stdout);

    // Notify processes to finish and exit.
    for (i = 0; i < WORKER_COUNT; i++) {
        if (workers[i].s != -1) {
            unsigned char byte = OP_EXIT;
            send(workers[i].s, &byte, 1, 0);
            close(workers[i].s);
        }
    }

    return 0;

err:
    // Close the sockets. Child processes will exit by themselves when they are detected to close.

    for (i = 0; i < WORKER_COUNT; i++) {
        if (workers[i].s != -1) {
            close(workers[i].s);
        }
    }

    return 1;
}