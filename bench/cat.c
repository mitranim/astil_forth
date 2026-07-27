// BOT-GENERATED

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int copy(FILE *inp, const char *name) {
  unsigned char buf[64 * 1024];
  size_t        len;

  while ((len = fread(buf, 1, sizeof(buf), inp))) {
    if (fwrite(buf, 1, len, stdout) != len) {
      perror("stdout");
      return 1;
    }
  }

  if (ferror(inp)) {
    perror(name);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc == 1) {
    if (copy(stdin, "stdin")) return 1;
  }

  for (int ind = 1; ind < argc; ind++) {
    if (!strcmp(argv[ind], "-")) {
      if (copy(stdin, "stdin")) return 1;
      continue;
    }

    FILE *input = fopen(argv[ind], "rb");
    if (!input) {
      perror(argv[ind]);
      return 1;
    }
    const int failed = copy(input, argv[ind]);
    if (fclose(input) && !failed) {
      perror(argv[ind]);
      return 1;
    }
    if (failed) return 1;
  }

  if (fflush(stdout)) {
    perror("stdout");
    return 1;
  }
  return 0;
}
