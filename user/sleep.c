#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
void main(int x) {
  sleep(x);
  exit();
}