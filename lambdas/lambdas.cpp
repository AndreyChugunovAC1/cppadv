#include <utility>
#include <iostream>

struct S {
  int s = 10;

  auto method() {
    auto f1 = [*this]() mutable {
      this->s = 20; // changes locally
      return this->s;
    };
    auto f2 = [this]() {
      return this->s;
    };
    // ...
    auto f3 = [&ss = s]() {
      return /* this-> */ss;
    };
    return f1() + f2(); // 30, 
  }
};

void test_compiles() {
  int a, b, c;
  auto l0 = []{};
  auto l1 = [](){};
  auto l2 = [](int x){ /* ... */ };
  auto l3 = [a](){}; // по значению
  auto l4 = [&a](){}; // по ссылке
  auto l5 = [=](){}; // все по значению
  auto l6 = [&](){}; // все по ссылке
  auto l7 = [=, &b](){}; // все по значению, b по ссылке
  auto l8 = [&, b](){}; // все по ссылке, b по значению
  auto lm = [am = std::move(a)](){}; // перемещаем a
  auto ln = [al = a](){}; // переименование
  auto ln2 = [&al = a](){}; // переименование
}

int main() {
  // using slam = decltype([]() static { return 20; });
  // std::cout << slam::operator() << "\n";
  std::cout << S{}.method() << "\n";
}