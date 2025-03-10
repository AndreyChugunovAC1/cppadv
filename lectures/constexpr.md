# Compile-time evaluation

__Проблема:__
```cpp
sized_storage<max(sizeof(T1), sizeof(T2))> ...
```
Раньше так написать было нельзя: max - runtime функция.

Даже `numeric_limits<int>::max()` раньше нельзя было вызвать в _compile-time_.

Когда нужны _compile-time_ вычисления:
* шаблоны
* размер массива (не `VLA`)
* `if constepr`
* `case` выражения

Можно писать что-то вроде:
```cpp
template<typename T, T x, T y>
struct max_struct {
  static T const value = x > y ? x : y;
};
```
но это:
1) Overhead по классам - плохо для компилятора
2) Нечитабельно и неудобно писать

## Инициализация глобальных переменных
```cpp
int f() { return 20 + 10; }
int x = 10 + 20;
int y = f(); // runtime
```
До входа в `main` приходится вычислять всю эту фигню - предпочтительнее по максимуму посчитать все на момент компиляции.

$\triangle$ В одном файле переменные инициализируются в порядке в каком они написаны. Порядок между файлами не определен.

$\triangle$ `std::cin`, `std::count` раньше инициализировались с подсчетом ссылок, чтобы не инициализироваться по несколько раз.

$\triangle$ Есть _gperf_ - кодогенератор хэш функций у GNU. Сейчас можно просто использовать `constexpr`.

$\triangle$ `std::regexp` может во время компиляции может сгенерировать конечный автомат для быстрого разбора выражений.

## constexpr
1) ключевое слово, которое намекает компилятору о возможности вычислить функцию в _compile-time_.
2) ключевое слово, которое заставляет считать значение переменной в _compile-time_.

```cpp
// not constexpr function:
int f() { return 30; }
constexpr int x = f(); // error
```

Изначально можно было писать только однострочные `constexpr` функции. Правила постепенно ослаблялись, и сейчас можно почти все.

Даже
* Псевдовыделение памяти
* Работа с указателями
* `constexpr` лямбды
* Функции которые не всегда бросают (при том throw недопустим)
* Динамические аллокации (обязательно удалять надо)
  * как следствие: `constexpr` у `vector`, аллокаторы (`std::allocator`) и прочее.
* Работа со структурами без паддинга (сам паддинг считается неясно как, надо без него)
```cpp
struct S1 { // may be bad
  int16_t a;
  // empty 16 bits
  int32_t b;
};

struct S2 { // OK
  int16_t a;
  int16_t b;
  int32_t c;
};
```

__Eliding__ - замена динамической алокации на аллокацию на стеке.
```cpp
delete new int(30); // can elide
operator delete(operator new(4)); // no elide
```

Что нельзя в _compile-time_:
* Просто потому что нет proposal:
  * goto
  * Достигающийся `throw`
  * virtual base
* Нельзя потенциально никогда:
  * функции без определения
  * inline assembly
* Пока не понятно почему нельзя:
  * `reinterpret_cast` (но есть `std::bit_cast`)
  * placement new (но можно `std::construct_at`)