# std::variant

Идея с закэшированным значением:
```cpp
if (!opt) {
  opt = compute(...);
}
```

(Если отдельно хранится) `compute` после подсчета не нужен. Его следует хранить тогда, когда он еще не использован.
```cpp
if (variants.index() == 0) { // хранится compute
  variants = get<0>(variants)(...); // "перещелкиваем"
}
```

## Про variant
`get<index>` vs `get<type>`. `get<type>` не всегда возможен. Почему: в `variant` допустимо повторение типов (ради шаблонного кода).

Конструирование:
```cpp
variant<A, B, A> v0(in_place_index<0>, A());
variant<A, B, A> v1; // default initialize 0 variant
```

Обращение:
```cpp
get<A>(v0) // error
get<0>(v0) // ok
get<2>(v0) // ok

get<B>(v0) // ok
get<1>(v0) // ok
```

## паттерн visitor
Код который кому-то нужен, но медленный:
```cpp
struct base {};
struct der1 : base {}
struct der2 : base {}

void func(base* b) {
  if (der1* d = dynamic_cast<der1*>(b)) {...}
  else if (der2* d = dynamic_cast<der2*>(b)) {...}
  else {...}
}
```
Вариация:
```cpp
struct base { virtual void visit() {...} };
struct der1 : base { void visit() override {...} }
struct der2 : base { void visit() override {...} }

void func(base* b) {
  b->visit();
}
```

Чем это плохо: нам приходится руками реализовывать все эти методы. Что если есть необходимость работать с библиотечными классами?

```cpp
struct visitor {
  virtual void visit(der1 const &) {...}
  virtual void visit(der2 const &) {...}
};
struct base { virtual void accept(visitor) {...} };
struct der1 : base { void accept(visitor) override {...} }
struct der2 : base { void accept(visitor) override {...} }

// ours:
struct my_visitor : visitor {
  void visit(der1 const &) override {...}
  void visit(der2 const &) override {...}
}

void func(base* b) {
  b->accept(my_visitor{});
}
```

Можно делать это все чуть статичнее (все равно придется на тип проверять):

```cpp
struct A {};
struct B {};

struct visitor {
  void operator()(A const&) const {...}
  void operator()(B const&) const {...}
};

int main() {
  std::variant<A, B> v = ...;
  std::visit(visitor{}, v);
}
```

Можно даже делать все возможные переборы. То есть реализовать 4 метода от 2 аргументов у `visitor` и вызвать `std::visit(visitor{}, varAB, varCD)` и далее.

__Хаки:__
```cpp
template<typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// deduction for C++17 (?)
template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>
...
std:visit(overloaded(
  [](A const&){},
  [](B const&){}
), varAB);
```

Самый нормальный способ:
```cpp
std::visit([](auto const&) {}, varAB);
```

__Куча фактов про `variant`:__
* `std::holds_alternative<A>(varABC)`
* Если первый тип не имеет конструктора по умолчанию, то можно использовать `std::variant v<std::monostate, A, B>`. Иначе создается элемент первого типа.
* `operator=`. v = A(), затем v = B(). Что если __копирование бросает__? 
  * у `boost` есть heap-backup variant. То есть он может повыделять память чтобы аккуратнее копировать/мувать.
  * Можно хранить 2 storage.
  * Гарантия непустоты.
  * Частично валидное состояние. (такое уже есть: `var.valueless_by_exception()` - возвращает false только есть есть значение)
* `variant` не умеет сравнивать типы напрямую (иначе $O(n^2)$ вариантов при компиляции) - он лексикографически сравнивает пары (`v.index()`, `get<v.index()>(v)`)
```cpp
variant<int, float> vgreater = 3.0f;
variant<int, float> vless = 1;
cout << (vless > vgreater) << "\n"; // true
```
