constexpr int x = 10;

constexpr void test1() {
  int x;
  int* p = &x;
  int y = *p;
}

#if 0 // error: constexpr function never produces a constant expression
constexpr void thrower() {
  throw int(10);
}
#endif

constexpr int thrower2(bool flag) {
  if (flag) {
    throw int(10);
  } else {
    return 30;
  }
}

constexpr void test2() {
  int x[] = {1, 2, 3};
  int* p = &x[0];
  int y = *p;
  // try {
  //   thrower();
  // } catch(...) {
  //   y = 100;
  // }
  int z = thrower2(false);
  // error while compiling:
  //   int w = thrower2(true);
}

int main() {
  test1();
  test2();
}