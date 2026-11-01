// Test runner entry point for hwtest.h. Linked once per test binary.
#include "hwtest.h"

int main(int argc, char **argv) {
  const char *only = argc > 1 ? argv[1] : nullptr;
  int passed = 0, failed = 0;
  for (const hwtest::Case &c : hwtest::registry()) {
    if (only && !std::strstr(c.name, only)) continue;
    std::fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
      alarm(5);
      sim::reset();
      c.fn();
      std::_Exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
      std::fprintf(stderr, "    timed out after 5 s (infinite loop in sketch?)\n");
    std::printf("  %s  %s\n", ok ? "PASS" : "FAIL", c.name);
    ok ? passed++ : failed++;
  }
  std::printf("\n%d passed, %d failed\n", passed, failed);
  return failed ? 1 : 0;
}
