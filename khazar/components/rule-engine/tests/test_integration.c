#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define SOCK_PATH "/tmp/test-ai-rule-engine.sock"
#define MAX_BUF 65536

static int tests_passed = 0;
static int tests_failed = 0;

static int connect_socket(void) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    for (int retry = 0; retry < 30; retry++) {
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) return fd;
        usleep(100000);
    }
    close(fd);
    return -1;
}

static int send_recv(int fd, const char *msg, char *buf, size_t bufsz) {
    if (write(fd, msg, strlen(msg)) <= 0) return -1;
    usleep(100000);
    ssize_t n = read(fd, buf, bufsz - 1);
    if (n > 0) { buf[n] = '\0'; return 0; }
    return -1;
}

pid_t start_daemon(void) {
    /* Create a temp config for the test */
    FILE *cf = fopen("/tmp/test-ai-rule-engine.toml", "w");
    fprintf(cf, "socket_path=\"%s\"\n", SOCK_PATH);
    fprintf(cf, "max_connections=64\n");
    fprintf(cf, "enable_cache=true\n");
    fprintf(cf, "enable_regex=true\n");
    fprintf(cf, "enable_tokens=true\n");
    fprintf(cf, "enable_intent_table=true\n");
    fprintf(cf, "enable_alias=true\n");
    fprintf(cf, "enable_fuzzy=true\n");
    fprintf(cf, "daemonize=false\n");
    fprintf(cf, "log_file=\"/tmp/ai-rule-engine-test.log\"\n");
    fprintf(cf, "log_level=3\n");
    fclose(cf);

    pid_t pid = fork();
    if (pid == 0) {
        unlink(SOCK_PATH);
        execl("./ai-rule-engine", "ai-rule-engine", "/tmp/test-ai-rule-engine.toml", NULL);
        exit(1);
    }
    return pid;
}

int main(void) {
    printf("\n==================== INTEGRATION TEST ====================\n");
    printf("            AI Rule Engine IPC Integration\n");
    printf("==========================================================\n\n");

    unlink(SOCK_PATH);

    pid_t pid = start_daemon();
    if (pid < 0) {
        printf("FAIL: daemon basladila bilmedi\n");
        return 1;
    }
    usleep(500000);

    int fd = connect_socket();
    if (fd < 0) {
        printf("FAIL: socket baglanmadi\n");
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        return 1;
    }

    char buf[MAX_BUF];

    /* 1: Ping */
    printf("TEST: ping ... ");
    if (send_recv(fd, "{\"type\":\"ping\"}", buf, sizeof(buf)) == 0 && strstr(buf, "alive")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    /* 2: Version */
    printf("TEST: version ... ");
    if (send_recv(fd, "{\"type\":\"version\"}", buf, sizeof(buf)) == 0 && strstr(buf, "version")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL\n"); tests_failed++; }

    /* 3: Metrics */
    printf("TEST: metrics ... ");
    if (send_recv(fd, "{\"type\":\"metrics\"}", buf, sizeof(buf)) == 0 && strstr(buf, "total_requests")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL\n"); tests_failed++; }

    /* 4: firefox ac */
    printf("TEST: firefox ac ... ");
    if (send_recv(fd, "{\"id\":1,\"type\":\"request\",\"query\":\"firefox ac\"}",buf,sizeof(buf)) == 0 &&
        strstr(buf, "open_application") && strstr(buf, "firefox")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    /* 5: wifi sondur */
    printf("TEST: wifi söndür ... ");
    if (send_recv(fd, "{\"id\":2,\"type\":\"request\",\"query\":\"wifi sondur\"}",buf,sizeof(buf)) == 0 &&
        strstr(buf, "network_disable")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    /* 6: sistemi yenile */
    printf("TEST: sistemi yenile ... ");
    if (send_recv(fd, "{\"id\":3,\"type\":\"request\",\"query\":\"sistemi yenile\"}",buf,sizeof(buf)) == 0 &&
        strstr(buf, "system_update")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    /* 7: telegram ac */
    printf("TEST: telegram ac ... ");
    if (send_recv(fd, "{\"id\":4,\"type\":\"request\",\"query\":\"telegram ac\"}",buf,sizeof(buf)) == 0 &&
        strstr(buf, "open_application") && strstr(buf, "telegram")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    /* 8: steam qurasdir */
    printf("TEST: steam qurasdir ... ");
    if (send_recv(fd, "{\"id\":5,\"type\":\"request\",\"query\":\"steam qurasdir\"}",buf,sizeof(buf)) == 0 &&
        strstr(buf, "install_package")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    /* 9: unknown */
    printf("TEST: unknown query ... ");
    if (send_recv(fd, "{\"id\":6,\"type\":\"request\",\"query\":\"xyz_non_existent\"}",buf,sizeof(buf)) == 0 &&
        strstr(buf, "unknown")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    /* 10: reload */
    printf("TEST: reload ... ");
    if (send_recv(fd, "{\"type\":\"reload\"}", buf, sizeof(buf)) == 0) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL\n"); tests_failed++; }

    /* 11: add_rule runtime */
    printf("TEST: add_rule (runtime) ... ");
    if (send_recv(fd, "{\"type\":\"add_rule\",\"rule_type\":\"intent\",\"data\":\"testapp ac|open_application|testapp|ac\"}",buf,sizeof(buf))==0 &&
        strstr(buf, "rule_added")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    /* 12: runtime rule lookup */
    printf("TEST: runtime rule lookup ... ");
    if (send_recv(fd, "{\"id\":7,\"type\":\"request\",\"query\":\"testapp ac\"}",buf,sizeof(buf)) == 0 &&
        strstr(buf, "open_application") && strstr(buf, "testapp")) {
        printf("PASS\n"); tests_passed++;
    } else { printf("FAIL (%s)\n", buf); tests_failed++; }

    close(fd);
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
    unlink(SOCK_PATH);
    unlink("/tmp/test-ai-rule-engine.toml");
    unlink("/tmp/ai-rule-engine-test.log");

    printf("\n==========================================================\n");
    printf("INTEGRATION NETICE: %d PASS, %d FAIL\n", tests_passed, tests_failed);
    printf("==========================================================\n\n");

    return tests_failed > 0 ? 1 : 0;
}
