#include <type_traits>
auto auto_good(int x) {
  if (x == 0)
    return 0; 
  return auto_good(x - 1) + x;
}

#if 0 // can not guess the type
auto auto_bad(int x) {
  if (x > 0) 
    return auto_bad(x - 1) + x;
  return 0;
}
#endif

int main() {
  // std::declval<int>();
  const int x = 10;
  decltype(x) y = 20; // const !
}