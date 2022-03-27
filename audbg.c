#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>

int main(int argc, char **argv, char **envp) {
    if (argc < 2) {
        fprintf(stderr, "example usage: %s /usr/bin/ls -R / "
                "| aplay -c2 -r44100 -fS16_LE\n", argv[0]);
        exit(0);
    }
    int pid = fork();
    if (!pid) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        close(0); close(1); close(2); // prevent child from messing with terminal
        execve(argv[1], argv+1, envp);
        fprintf(stderr, "execve failed\n");
        exit(1);
    }
    fprintf(stderr, "child pid %d\n", pid);
    if (ptrace(PTRACE_ATTACH, pid, 0, 0)) {
        fprintf(stderr, "failed to attach\n");
        exit(1);
    }
    if (waitpid(pid, 0, WSTOPPED) == -1) {
        fprintf(stderr, "failed to wait\n");
        exit(1);
    }
    struct user_regs_struct regs;
    while(1) {
        ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
        if (waitpid(pid, 0, WSTOPPED) == -1) {
            fprintf(stderr, "error (maybe process exited)\n");
            exit(1);
        }
        ptrace(PTRACE_GETREGS, pid, 0, &regs);

        // generate audio samples on standard output.
        // here we are just taking the low 16 bits of some register.
        int s = regs.rax;
        write(1, &s, 2);
    }
}
