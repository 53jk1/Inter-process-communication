// $ gcc -Wall -Wextra pipe.c -o pipe
// $ ./pipe

#define _GNU_SOURCE // The program uses functions specific to GNU/Linux.

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1
#define UNUSED(a) ((void)a)

typedef struct {
    int in;
    int out;
    int err;
} process_standard_io;

pid_t spawn_process(process_standard_io *pipes, char *program, char **argv, char **envp) {
    // Create pipes for standard input, output and error output.
    int in[2], out[2], err[2];
    pipe(in);
    pipe(out);
    pipe(err);

    // Start a new process.
    pid_t child = fork();
    if (child == 0) {
        // A branch of a child process.

        // Close unused pipe ends, i.e.:
        // - ending from the notation in the case of in,
        // - the read tail in the case of out and err.
        close(in[PIPE_WRITE_END]);
        close(out[PIPE_READ_END]);
        close(err(PIPE_READ_END));

        // Set the descriptors of the corresponding in, out, and err endings to be those
        // used by stdin (0), stdout (1), and stderr (2).
        // The dup2 function makes a copy of the descriptor. After the copy is made,
        // the old descriptor is no longer needed, so you can close it.
        dup2(in[PIPE_READ_END], STDIN_FILENO);
        dup2(out[PIPE_WRITE_END], STDOUT_FILENO);
        dup2(err[PIPE_WRITE_END], STDERR_FILENO);
        close(in[PIPE_READ_END]);
        close(out[PIPE_WRITE_END]);
        close(err[PIPE_WRITE_END]);

        // At this point, the appropriate pipe endings are hooked to the child process's
        // stdin, stdout, and stdeerr. So you can run the target program that will
        // inherit them.
        execve(program, argv, envp);

        // If the execution gets to this point, something has gone wrong.
        perror("child execve failed");

        // In some cases, we cannot be sure of the state in which a child process starts
        // "cloned" with a fork. In particular, we may not know what state the mutexes
        // are in (potentially another thread of the parent process may have occupied
        // the mutex before calling for, and since only the main thread is copied, this
        // mutex will never be released). We may also not know if the main process (or
        // one of the libraries used by it) has not registered with the atexit function
        // a procedure to be called at the end of the process (i.e. return from the main
        // function or call to the exit function) and which it might want to start
        // (already taken and never released) mutex, which would lead to starvation
        // and permanent blockage of the child process when trying to exit.
        // Therefore, it is safer to use the _exit function that ignores the
        // atexit registered procedures.

        _exit(1);
    }

    // Parent branch.

    // Close unused pipe ends.
    close(in[PIPE_READ_END]);
    close(out[PIPE_WRITE_END]);
    close(err[PIPE_WRITE_END]);

    // Rewrite the remaining pipe descriptors into the output structure and come back.

    pipes->in = in[PIPE_WRITE_END];
    pipes->out = out[PIPE_READ_END];
    pipes->err = err[PIPE_READ_END];
    return child;
}

int main(int argc, char **argv, char **envp) {
    UNSUED(argc);
    UNUSED(argv);

    process_standard_io pstdio;
    char *process_path = "/usr/bin/find";
    char *process_argv[] = {
        process_path,
        "/etc",
        "-name", "passwd",
        NULL
    };
    pid_t child = spawn_process(&pstdio, process_parg, process_argv, envp);

    // Wait for the child process to finish. All data printed to standard output and
    // error output will still be in the kernel buffers belonging to the appropriate pipes.

    waitpid(child, NULL, 0);

    // List data from standard outputs. For this purpose, I use with the splice function,
    // which copies data directly between two pipes, without the need to manually load them
    // into the process memory from the source pipe and write to the target pipe. Since
    // splice uses low-level descriptors (not FILE object), I'm using the fileno function
    // to get the low-level standard output descriptors for the current process. 
    // Alternatively, I could use the constant 1 as the standard output in this process 
    // will definitely be on that descriptor.
    puts("Child's STDOUT:"); fflush(stdout);
    splice(pstdio.out, NULL, fileno(stdout), NULL, 1024, 0);

    puts("\nChild's STDERR:"); fflush(stdout);
    splice(pstdio.err, NULL, fileno(stdout), NULL, 1024, 0);

    putchar('\n');

    // Close the streams.
    close(pstdio.in);
    close(pstdio.out);
    close(pstdio.err);

    return 0;
}