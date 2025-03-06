#include <concepts>
#include <iostream>
#include <type_traits>

template <std::integral auto value> void print() { std::cout << value << "\n"; }

// template<std::integral auto arg, std::integral auto a>
// concept greater_than = (arg > a);

// template<int x, long y>
// requires greater_than<y, x>
// void f() {}

template <typename T>
concept callable_int_int = requires(T x) {
  { x(0) } -> std::same_as<int>;
};

struct callable_int_int_yes {
  int operator()(int x) { return x * x; }
};

struct callable_int_int_no {
  void operator()(float x) {}
};

template <callable_int_int T> void call(T obj, int x) {
  std::cout << obj(x) << "\n";
}

template <typename T, typename... Args>
struct callable_impl {
  decltype(std::declval<T>()(std::declval<Args>()...));
  static constexpr bool value = true;
};

// template <typename T, typename... Args>
// struct callable_impl<T, Args...> {
//   static constexpr bool value = false;
// };

int main() {
  // std::is_invocable
  print<3>();
  print<30>();

  call(callable_int_int_yes{}, 19);
  // call(callable_int_int_no{}, 19);
}