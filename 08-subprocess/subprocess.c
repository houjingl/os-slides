#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void check_error(int ret, const char *message) {
    if (ret != -1) {
        return;
    }
    int err = errno;
    perror(message);
    exit(err);
}

static void parent(int in_pipefd[2], int out_pipefd[2], pid_t child_pid) {
    close(out_pipefd[0]);
    
    const char* str = "Testing \n";
    int len = strlen(str);
    int bytes_written = write(out_pipefd[1], str, len);
    check_error(bytes_written, "write");
    
    close(out_pipefd[1]);

    close(in_pipefd[1]); // Close the fd I am not using -> write end of the pipe
    
    //read from the read end of the pipe
    char buffer[4096];
    int bytes_read = read(in_pipefd[0], buffer, sizeof(buffer));
    printf("read: %.*s", bytes_read, buffer);

    close(in_pipefd[0]);

    int wstatus = 0;
    check_error(waitpid(child_pid, &wstatus, 0), "wait");
    // Parent should be responsible for checking child's status and making sure it dies.
}

static void child(int in_pipefd[2], int out_pipefd[2], const char *program) {
    dup2(in_pipefd[1], 1);
    //this dup 2 makes fd 1 to point to the write end of the pipe !
    //so that if we dont read from the read end, we will not get any output from child!
    close(in_pipefd[0]); 
    close(in_pipefd[1]); //close the original fd so that only 1 points to the write end of the pipe
    //Always close unused fd!
    
    dup2(out_pipefd[0] ,0);
    close(out_pipefd[1]);
    close(out_pipefd[0]);

    execlp(program, program, NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return EINVAL;
    }

    int in_pipefd[2] = {0};

    check_error(pipe(in_pipefd), "pipe");
    // Created a pipe named inpipe
    // fd 3 and 4 pointing to the read and write end of the pipe

    int out_pipefd[2] = {0};

    check_error(pipe(out_pipefd), "pipe");

    pid_t pid = fork();
    if (pid > 0) {
        parent(in_pipefd, out_pipefd, pid);
    }
    else {
        child(in_pipefd, out_pipefd, argv[1]);
    }



    return 0;
}
