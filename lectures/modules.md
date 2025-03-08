# Модули
Проблемы `#include<>`:
* Загрузка целого текста в файл, соответственно, долгая компиляция
* Если заголовочный файл изменяется, все файлы, которые его включают, должны быть перекомпилированы
* Что не так в коде:
```cpp
#pragma once
// header.h
void foo() {}
```
* Надо написать `inline`. То есть нужна какая-то дурацкая дисциплина.
* Порядок `#include` может влиять на поведение программы (пример: `Windows.h` и `WinSocket.h`)
* Хэдеры могут содержать в себе больше чем в них есть
* _Препроцессор - зло_ (цитата Страуструпа).

## Как писать
Расширение для файла с модулем: `.ixx`. При его компиляции генерируются два файла: `.obj` (машинный код), и `.ifc` (интерфейс модуля, его быстро разбирать).
```cpp
// sum.ixx либо sum.cppm
export module sum;
export int sum(int a, int b) {
  return a + b;
}
// main.cpp
import sum;
int main() {
  sum(2, 3);
}
```
* `export ...` - то что видно извне. `export {...}`.
* `module;` - global module fragment. Нужен для подключения хэдеров. В глобальном модульном фрагменте нельзя объявлять экспортируемые сущности (они должны быть объявлены после export module)
  * Изолирует макросы
  * Избегание дублирования кода при использовании общих заголовочных файлов

```cpp
// propogate imported modules
export {
  import algorithm;
}
// the same:
export import algorithm;
```

## module implementation unit
Проблема:
```cpp
// sum.ixx
int sum(int a, int b) {
  // 100mb of code
}

// user.ixx
import sum;

void foo() {
  // 100mb of code
}
```
Этот код будет компилироваться даже дольше чем если бы мы нормально пользовались хэдерами.

Решение: разнести модуль на 2 части: _module implementation unit_ и _primary module interface unit_.
```cpp
// primary module interface unit
export module sum;

int sum(int a, int b);

// module implementation unit:
module sum;

int sum(int a, int b) {
  // 100mb of code
}
```
Теперь все ок.

В качестве альтернативы можно написать `module :private`:
```cpp
// file.ixx:
// primary module interface unit
export module sum;

int sum(int a, int b);

module :private;

int sum(int a, int b) {
  // 100mb of code  
}
```

## partition
```cpp
export module mod:part1;
// or
module mod:part2;
```

Вот такой пример:
```cpp
export module math; // module interface unit

import :arithmetic; // Импорт части arithmetic
import :geometry;   // Импорт части geometry

export {
  using arithmetic::add;
  using arithmetic::subtract;
  using geometry::area_of_circle;
}
```

## Про ODR
ODR. Модули владеют функциями и классами, которые в них объявлены. Например, если мы подключим два модуля с одной и той же функцией, то всё будет компилироваться, но мы не сможем вызвать эту функцию.
 
## Еще
Почему нельзя сделать единый формат модуля для всех компилаторов?

### visible/reachable
```cpp
export module test;

// reachable struct
/* export */ struct pair {
  int x;
  int y;
};

export pair make_pair(int a, int b) {
  return { a, b };
}
```
