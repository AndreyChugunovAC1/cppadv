# Концепты

Плохие примеры:
```cpp
vector<void> v0;
vector<int&> v1;
vector<int()> v2;
vector<int const> v3;
vector<no_dtor> v3;
```
Это и не работает, как и надо, но сообщения об ошибках чудовищных размеров.
В случае с концептами ошибки становятся прозрачными.

```cpp
assign(size_t n, T const& value);
template<typename It>
assign(It a, It b);
```
Для `vector<size_t>().assign(10, 20)` будет выбрана вторая перегрузка. (не SFINAE-friendly)

Старое SFINAE решение:
```cpp
template<typename = std::enable_if_t<condition>>
```
* Это сложный языковой механизм (абьюз языкового механизма)
* Это нечитабельно, boilerplate
* Ошибка выглядит ужасно
* Сложно работать с более частными случаями (`enable_if<a>` и `enable_if<a && b>` для компилятора одно и то же).

## implicit, explicit concept
Пример:
```cpp
struct mytype {};
operator<(mytype const&, mytype const&)
```
Еще пример:
```cpp
struct shape { void draw(); };
struct gun { void draw(); };
```
Синтаксические и семантические требования:
синтаксически _forward_ и _input_ итераторы одинаковые: у них одинаковые операции, семантически - нет

В стандартной библиотеке есть 2 такие ситуации: итераторы, borrowed ranges.

В С++ договорились что все концепты синтаксические (implicit).

```cpp
void func(input_iterator auto& it, integral auto distance) { ... }
```
_definition-checking_: нужен чтобы ограничения были и у вызываемой стороны. Чтобы не написать it += 1 если нам всегда везет, что передают `random_access_iterator` например.

Проблема. 
Нужно вывести на экран что-то:
```cpp
void f(T const& x) { cout << x << "\n"; }
```
Если навесить концепт, то и вызывающая сторона будет обязана навесить этот концепт. Иначе как бы не совсем _definition-checking_.

Поэтому пошел он нафиг

## concept_map
Синтаксические условия на тип, но допустимо переименование. Нпример, `vector` можно было бы воспринимать как стек, просто вместо `push` был бы `push_back`, а вместо `pop` - `pop_back`.

Тоже не вошел в язык.

## Синтаксис
__Использование и написание:__
```cpp
template<typename T>
concept req = std::is_nothrow_move_assignable_v<T>;

template<typename T>
concept req = std::is_nothrow_move_assignable_v<T>;

template<typename T>
requires req<T>
void func(T);

template<req T>
void func(T);

void func(req auto);
```

__Шаблоны по значению:__
```cpp
template<req auto X>

template<auto X>
requires req<X>
```

Requirement-clause: `requires expr`
Внутри requires expr:
```cpp
template <typename T>
concept callable_int_int = requires(T x) {
  { x(0) } -> std::same_as<int>;
};
```
* Валидность свойства выражения - _compound requirement_
* Наличие функции - _simple requirement_
* Наличие типа (зависимого) - _type requirement_

## Более/менее специализированное
```cpp
concept A // atomic
concept B // atomic
concept C = A & B // more specialized
concept D = C && true // more cpecialized
concept D = C || false // less cpecialized
/// etc...
```

## Прочее
### `same_as`:
```cpp
template<typename A, typename B>
concept half_same_as = std::is_same_v<A, B>;
// для симметрии:
template<typename A, typename B>
concept same_as = half_same_as<A, B> && half_same_as<B, A>;
```
### Конфуз:
```cpp
template<typename T>
requires requires(T x) { ... }
```

### SFINAE иногда не SFINAE-friendly:
```cpp
template<typename A, typename B>
struct pair {
private:
  A a; B b;
  pair(A _a, B _b) : a(_a), b(_b) {}
}
```
`is_default_constructible<pair>` дает true (конструктор же есть), при этом понятно что конструктор может не скомпилироваться.
```cpp
template<typename A, typename B>
struct pair {
private:
  A a; B b;
  template<typename = std::enable_if_t<...>>
  pair(A _a, B _b) : a(_a), b(_b) {}
}
```
Все равно hard ошибка - с концептами ее не будет.

### Предпочтение при наследовании
```cpp
struct A {};
struct B : A {};
struct C : B {};
f(A const&);
f(B const&);
f(B{}) // лучше подходит f(B const&)
f(C{}) // лучше подходит f(B const&)
```



