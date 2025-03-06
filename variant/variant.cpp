#include <iostream>
#include <variant>

struct throwing_cast {
  operator float() const {
    throw int(-1);
  }
};

int main() {
  std::variant<int, float> v; // int
  try {
    v = throwing_cast{}; 
  } catch(...) {
    std::cout << v.valueless_by_exception() << "\n";
  }

  std::variant<int, float, char> v2 = 'a';
  auto visitor = [](auto const& x) {
    std::cout << typeid(x).name() << " " << x << "\n";
  };
  std::visit(visitor, v2);
  v2 = 10;
  std::visit(visitor, v2);
  v2 = 3.14f;
  std::visit(visitor, v2);

  std::variant<int, float, char> vl = 30.0f, vr = 100;
  std::cout << (vl > vr) << "\n"; // true
}