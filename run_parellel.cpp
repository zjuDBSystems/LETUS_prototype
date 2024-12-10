#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFFER_SIZE 1024

// 用于读取和打印子进程输出的函数
void read_process_output(FILE *fp, const char *name) {
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("%s: %s", name, buffer);
    }
}

int run_process(const char *cmd, int *out_fd, int *err_fd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {
        // 子进程
        close(out_fd[0]);
        close(err_fd[0]);

        dup2(out_fd[1], STDOUT_FILENO);  // 重定向标准输出
        dup2(err_fd[1], STDERR_FILENO);  // 重定向标准错误输出

        execlp(cmd, cmd, (char *)NULL);   // 执行命令
        perror("execlp failed");
        exit(1);
    }

    return pid;
}

int main() {
    // 创建管道用于捕获子进程的输出
    int verify_out[2], verify_err[2];
    int main_out[2], main_err[2];

    if (pipe(verify_out) == -1 || pipe(verify_err) == -1) {
        perror("pipe failed for verify");
        return -1;
    }

    if (pipe(main_out) == -1 || pipe(main_err) == -1) {
        perror("pipe failed for main");
        return -1;
    }

    // 启动 verify 进程
    pid_t verify_pid = run_process("./verifyClient --verify-only", verify_out, verify_err);
    if (verify_pid < 0) {
        printf("启动 verify 进程失败\n");
        return -1;
    }


    pid_t main_pid = run_process("./verifyClient", main_out, main_err);
    if (main_pid < 0) {
        printf("启动主程序进程失败\n");
        kill(verify_pid, SIGTERM);  // 终止 verify 进程
        return -1;
    }

    FILE *verify_out_fp = fdopen(verify_out[0], "r");
    FILE *verify_err_fp = fdopen(verify_err[0], "r");
    FILE *main_out_fp = fdopen(main_out[0], "r");
    FILE *main_err_fp = fdopen(main_err[0], "r");


    while (1) {
        read_process_output(verify_out_fp, "Verify");
        read_process_output(verify_err_fp, "Verify 错误输出");
        read_process_output(main_out_fp, "Main");
        read_process_output(main_err_fp, "Main 错误输出");

        int status;
        if (waitpid(verify_pid, &status, WNOHANG) > 0 && WIFEXITED(status)) {
            break;
        }
        if (waitpid(main_pid, &status, WNOHANG) > 0 && WIFEXITED(status)) {
            break;
        }

        usleep(100000);  
    }

    int status;
    waitpid(verify_pid, &status, 0);
    waitpid(main_pid, &status, 0);

    fclose(verify_out_fp);
    fclose(verify_err_fp);
    fclose(main_out_fp);
    fclose(main_err_fp);

    return 0;
}
