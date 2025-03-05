# Perfect Forwarding
Для чего нужен _perfect forwarding_:
```cpp
auto p = make_unique<A>(1, 2, 3);
// ...
vec.emplace_back(1, 2, 3);
```
__Зачем:__
* Создание объекта сразу там где надо (push_back будет пытаться перемещать его)
* Для типов, которые не умеют перемещатсья либо не _nothrow_ перемещаются

Зачем `make_unique`:
```cpp
int* p = new int(3);
unique_ptr<int> up1(p);
auto up2 = make_unique<int>(3);
```
* В первом случае нужно следить чтобы p никуда больше не делся.
* Передача параметра в функцию до C++17 _indeterminate-sequenced_ - порядок вычисления аргументов не гарантирован. Сейчас порядок строго определен и он такой, какой все ожидают.
```cpp
func(unique_ptr<int>(new int(3)), may_throw());
```

Чтобы понять, как должен работать forward нужно перебрать все варианты:
```cpp
void f(int);
void f(int&);
void f(int&&);
void f(int const&);
// ...
void g(/* ??? */) { 
  f(/* ??? */);
}
```
* Не лучший вариант пользоваться `template<typename T> .. T`
* Не получается и `template<typename T> .. const T&`

## Reference collapsing
||&|&&|
|-|-|-|
|&|&|&|
|&&|&|&&|
```cpp
using T = int&;
using U = T&; // тоже int&
```
## Deduction для T&& (universal reference/forward reference)
Фактически, дадим определение перечислением.
```cpp
template<typename T>
void g(T&&);

int a;
int const b;
g(1);            // int
g(a);            // int&
g((int const)1); // int const
g(b);            // int const&
```

Решение проблемы выше:
```cpp
template<typename T>
void g(T&& x) {
  f(static_cast<T&&>(x)); // ref collapsing
}
```
`static_cast` - решение, но считается, что его использование нечитабельно, поэтому есть `forward<T>`
```cpp
template<typename T>
T&& forward(T& x) {
  return static_cast<T&&>(x);
}
```
* не работает для rvalue (нужно для генерализированного кода)
* Сам выводит тип (правило вывода T для T&& не всегда подходят) - нужен no_deduce.

## Итог (с использованием remove_reference_t):
```cpp
template<typename T>
remove_reference_t<T>&& move(T&& x) {
  return static_cast<T&&>(x);
}

// rem_ref -> no deduce
template<typename T>
T&& forward(remove_reference_t<T>& x) {
  return static_cast<T&&>(x);
}

// rem_ref -> no deduce
template<typename T>
T&& forward(remove_reference_t<T>&& x) {
  return static_cast<T&&>(x);
}
```
Использование perfect forwarding:
```cpp
template<typename T, typename... Args> 
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(forward<Args>(args)...));
}
```

## "Perfect backwarding", decltype
```cpp
/* ??? */ g(T&& x) {
  return f(std::forward<T>(x));
}
```
### decltype
1) `decltype(name)` - тип имени
```cpp
int            x;
decltype(x)    y; // int y
decltype(S::s) z; // Тип который у S::s
```
2) `decltype(expr)` - тип выражения
```cpp
decltype(2 + 2) x = 2; // int x
decltype((x))   y = x; // int& y
```
`decltype` сохраняет константность

__Проблема:__
```cpp
decltype(f(T())) // не всегда работает
```
Не работает когда T не имеет конструктора по-умолчанию (любая ссылка, например), решение:
```cpp
template<typename T>
T&& declval() {
  assert(false, "can not be called in evaluating context");
}
```
`declval` - только в _non-evaluating-context_.

__Решение #1__ проблемы выше:
```cpp
template<typename T> //
decltype(f(declval<T>())) g(T&& x);
```

### Trailing return type
```cpp
auto function(Args... args) -> R;
```

__Решение #2__ проблемы выше:
```cpp
template<typename T>
auto g(T&& x) -> decltype(f(x));
```

### auto
... - вывод типа как `template<typename T> .. T`.
```cpp
template<typename T, typename U>
void f(T*, U);
// <=>
void f(auto*, auto);
```

Как возвращаемое значение:
* Смотрит на самый первый return в функции, если в разных return разные типы - ошибка. См. примеры кода.
```cpp
auto f(/* ... */); 
```

### decltype(auto)
... - полный вывод типа

```cpp
int const x;
decltype(auto) y = x; // int const&
```

__Решение #3__ проблемы выше:
```cpp
template<typename T> //
decltype(auto) g(T&& x);
```

## nullptr
0 - это нулевой указатель. Но это все же `int`.

Пример: `g()` - форвардит свои аргументы.
`g(/* указатель */ 0)` не написать.

Решение:
```cpp
struct nullptr_t {
  template<typename T>
  operator T*() const {
    return 0;
  }
};

nullptr_t nullptr;
```
Теперь форвардинг будет выводить по крайней мере указатель.

## lifetime extension
const ссылка продлевает жизнь объекта, на который она забинжена.
```cpp
string get_str() {
  return "rvalue string";
}
string const& str = /* prvalue */ get_str(); // OK
```
аналогично с rvalue-ссылками
```cpp
int xvalue();
int&& a = xvalue(); // OK
```

Просто пример 1:
```cpp
for (pair<int, int> const& entry : mapp) { /* ... */ }
```
* тут будут копирования, потому что на самом деле в map хранится `pair<int const, int>`.

Просто пример 2:
```cpp
// Нельзя auto& потому что там прокси объект "ссылка на бит"
for (auto&& e : vec_bool) { /* ... */ }
```

__Решение:__ Всегда писать `auto&&` для _read-write_ либо `auto const&` для _read-only_.











