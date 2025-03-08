#include <generator>
#include <iostream>

std::generator<int> generate_squares() {
  int i = 0;
  for (;;) {
    co_yield i * i;
    ++i;
  }
}

int main() {
  std::generator<int> gs = generate_squares();
  auto it = gs.begin();
  for (int i = 0; i < 10; i++) {
    std::cout << *it << "\n";
    ++it;
  }
  // ...
}
