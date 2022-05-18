#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
  struct statx stx{};
  statx(AT_FDCWD, argv[1], AT_EMPTY_PATH, STATX_ATIME | STATX_SIZE, &stx);

  std::cout << stx.stx_size << "\n";
  std::cout << stx.stx_blksize << "\n";
  std::cout << stx.stx_atime.tv_sec << "\n";
}