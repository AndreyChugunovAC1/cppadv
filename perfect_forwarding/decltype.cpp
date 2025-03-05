auto auto_good(int x) {
  if (x == 0)
    return 0; 
  return auto_good(x - 1) + x;
}

#if 0
auto auto_bad(int x) {
  if (x > 0) 
    return auto_bad(x - 1) + x;
  return 0;
}
#endif

int main() {
  const int x = 10;
  decltype(x) y = 20; // const !
}