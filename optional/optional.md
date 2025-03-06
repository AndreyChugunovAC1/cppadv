# std::optional

## Зачем
* Закэшированные значения
* Можно самому хранить `value` и `is_initialized` но это плохо читабельно
* Может не быть конструктора по умолчанию у типа `value`
* Компилятор может не подсказать ошибку, не понимая, что `value` и `is_initialized` связаны (не знает инвариант)

## Как писать
```cpp
template<typename T>
struct optional {
  bool is_initialized;
  T value;
}
```
Если нет конструктора по умолчанию:
```cpp
template<typename T>
struct optional {
  bool is_initialized;
  char value[sizeof(T)];
}
```
Проблемы с выравниванием:
```cpp
template<typename T>
struct optional {
  bool is_initialized;
  // alignas(T) char value[sizeof(T)];
  aligned_storage<sizeof(T), alignof(T)> value;
}
```
Проблема с `constexpr`, ведь в таком случае придется делать _placement new_ (его можно заменить на `construct_at`) и `reinterpret_cast`.
```cpp
template<typename T>
struct optional {
  bool is_initialized;
  union {
    T value;
  };
}
```
Вот это уже ок, пока без учета traits типа `T`.

## Как пользоваться
Создание вида `emplace_back`
```cpp
optional<mytype> opt(in_place, 1, 2);
opt.emplace(2, 3);
```
Создание пустого:
```cpp
optional<mytype> eopt1;
optional<mytype> eopt2(nullopt);
eopt3 = nullopt;
```
Сравнение с другими `optional`:
Пустой меньше непустого. В остальном сравнивает значения как надо.

Иногда лучше `unique_ptr`:
* Стабильность адреса
* non-moveable типы
* Большой sizeof(T)

Еще кейсы (случаи) когда optional хуже:
* `vector<optional<T>>` хуже чем `vector<bool>` + `vector<T>` по памяти .
* Иногда достаточно просто считать, что, скажем, -1 - это невалидное значение для `int`.

## noexcept
Бывает условный `noexcept`.
```cpp
void func() noexcept(false) {}
```
Зачем: пример с вектором, который требует чтобы `move` не бросал, при перевыделении памяти он не сможет гарантированно всех переместить и будет копировать. Поэтому хорошо бы, если бы `optional` работал с точки зрения исключений как и хранимый тип.
`noexcept` это:
1) спецификатор функции, внутри которого можно написать _compile-time_ условие при котором функция не бросает исключений
2) Проверка что выражение не бросающее исключений: `noexcept(T())`.

Соответственно возможен курьез: `noexcept(noexcept(expr))`.

## SFINAE-friendliness
Пример (о чем это):
```cpp
template<typename U>
optional(optional<U> const &) ...
```
Со стороны кажется, что любой optional умеет конвертироваться в любой другой, но это не так: на самом деле проверка на это просто будет проведена позднее. То есть речь про умение обертки вести себя как сам объект.

## Тривиальность
```cpp
struct a {
  a() = default; // trivially constructible
};
struct b {
  b() {} // non-trivially constructible
};
```

* `is_trivially_copy_constructible`
* `is_trivially_move_constructible`
* `is_trivially_copy_assignable`
* `is_trivially_move_assignable`
* `is_trivially_destructible`
* `is_trivially_default_constructible`

Зачем это нужно? Самый заметный пример: если тип тривиально копируемый, то фактически можно просто копировать его байты, а значит можно использовать `memcpy`. Также компилятор не будет делать лишних действий: вызовов операторов, консрукторов, деструкторов и прочее. 

`vector` из тривиальных типов намного дешевле расширяется.

Хороший пример: ассемблерного кода для unique_ptr раза в 3 больше чем кода для работы с указателем из-за нетривиальности.

Есть аттрибут `[[clang::trivial_abi]]` который делает некоторые оптимизации.

Есть идея _trivially\_reallocable_: если тип умеет тривиально делать move + destroy. ~~Таким примером, __кажется__, является `unique_ptr`. Ему достаточно просто побайтово перенести указатель.~~

_trivial_ $\subset$ _nothrow\_default_ $\subset$ _default_ $\subset$ остальное.

## еще
```cpp
explicit(condition) 
noexcept(condition)
```

Что пропагейтит `optional`:
* тривиальность операций, отсутствие операций, кроме тривиальности дефолтного конструктора (нужно инициализировать `is_initialized(false)` )
* noexcept
* explicit

Поскольку `optional` умеет конвертироваться в `bool`, его можно использовать так:
```cpp
if (auto x = get_optional()) {...}
```



