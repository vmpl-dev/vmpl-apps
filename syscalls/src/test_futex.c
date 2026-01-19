#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#define FUTEX_WAIT 0

int main() {
    int futex_var = 0;

    // Test futex system call
    int result = syscall(SYS_futex, &futex_var, FUTEX_WAIT, 0, NULL, NULL, 0);
    if (result == -1) {
        perror("Futex system call failed");
        exit(EXIT_FAILURE);
    }

    // Verify that the futex_var has been modified
    if (futex_var != 0) {
        printf("Futex system call succeeded. futex_var = %d\n", futex_var);
    } else {
        printf("Futex system call succeeded, but futex_var was not modified.\n");
    }

    return 0;
}