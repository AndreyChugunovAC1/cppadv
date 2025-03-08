# Многопоточность

см. примеры кода в `threads.cpp`

1) Хочется использовать процессоры по максимому.
2) Это не единственный способ. можно запустить много процессов (например, многие компиляторы именно "многопроцессовые")
3) Это позволяет логически разделить программу на части: например, игра отдельно воспроизводит звук, отдельно рендерит картинку или даже разные персонажи просчитываются на разных ядрах.

## std::thread
```cpp
std::thread th([](){
  std::cout << "Hello from thread\n";
});
th.join();
```
__Замечания:__
* std::cout безопасно использовать из разных потоков. Однако следует учитывать, что два разных оператора `<<` работают независимо.
* c `std::osyncstream(std::cout)` вывод не будет перемешиваться.

`join` - дожидается завершения и отвязывает _thread_ от потока ОС.

`detach` - отвязывает _thread_ от программы (_thread_ вместе с потокм ОС теперь вне нашего поля зрения). Лучше его не использовать, он _error-prone_.

Если забыть сделать `join`, то программа завершится с ошибкой (`std::terminate`). Буквально, деструктор `thread`:
```cpp
if (joinable())
  _STD terminate();
```

Замечания:
* методы: `joinable`, `detach`, `join`.
* Имеет конструктор по умолчанию, потом можно писать `th = std::thread(...)`, если `th` не _joinable_ (иначе - `std::terminate`).
* Нельзя копировать, можно только перемещать.
* `std::this_thread::get_id()`
* `std::this_thread::sleep_for(...)`

### std::jthread
... - улучшенная версия `thread`. В деструкторе он делает `join`. 

Почему так не работает `thread`: нехорошо, если в деструкторе он будет ждать завершения бесконечного потока.

Да `jthread` тоже ждал бы, но у него есть механизм остановки потока (_cancellation_):

```cpp
std::jthread th([](std::stop_token const& st){
  while (!st.stop_requested()) { ... }
})
```

### Принудительное завершение потока.
Возможно, хочется воспользоваться чем-то наподобие `th.terminate()` (такого нет) чтобы немедленно завершить поток. В Windows есть `TerminateThread`, однако, в документации к нему написано, что использовать его нужно крайне аккуратно и только если уверены во всем на свете.

Почему: очень часто инварианты подразумевают некоторое логическое завершение.
Собственно, вот пример (плохо, если поток завершится до `delete`):
```cpp
std::thread th([]{ delete new int(30); });
```

Принудительное завершение процесса при этом не так страшно: ОС следит за их ресурсами, поэтому она может аккуратно с ними разобраться.

## Многопоточные программы
### Закон Амдала
```
P - parallelable
S - sequenced
1 = P + S

n threads: P/n + S
```
иллюстрирует ограничение роста производительности вычислительной системы (ускорение не более чем в `p(n)` раз).

### Рассмотрим код
```cpp
int32_t accs[...];

void transfer(size_t from, size_t to, int32_t amount) {
  if (accs[from] < amount)
    throw runtime_exception("...");
  accs[from] -= amount;
  accs[to] += amount;
}
```
* Этот код с точки зрения C++ - один большой UB.
* Возможен _race condition_ - состояние гонки (ошибка проектирования).

### std::mutex (mutual exclusion)
... - фактически `bool`. Когда поток доходит до метода `m.lock()`, он либо блокирует `mutex`, либо наоборот, узнает что он заблокирован и ждет когда его разблокируют.

Когда поток завершил выполнение кода, для которого нужна была блокировка, надо вызвать `m.unlock()`.

Замечания:
* В Microsoft они называются _критическими секциями_. Мьютексом же у них называется объект ядра.
* В clang есть _thread-safety-alanysis_, который использует специальные аннотации.

__Решение #1:__
```cpp
int32_t accs[...];
mutex m;

void transfer(size_t from, size_t to, int32_t amount) {
  m.lock();
  if (accs[from] < amount)
    throw runtime_exception("...");
  accs[from] -= amount;
  accs[to] += amount;
  m.unlock();
}
```
* Если вылетит ошибка, то потоки навсегда зависнут. `mutex` работает без RAII.
* Нужна RAII обертка над ним. В stl есть `std::lock_guard`.

__Решение #2:__
```cpp
int32_t accs[...];
mutex m;

void transfer(size_t from, size_t to, int32_t amount) {
  std::lock_guard lg(m);

  if (accs[from] < amount)
    throw runtime_exception("...");
  accs[from] -= amount;
  accs[to] += amount;
}
```
* Это уже корректный C++ код.
* Это глупый код - в нем не только нет параллелизма, но он еще и медленнее работает чем однопоточная версия.
* Можно сделать мьютекс на каждый _account_. Мьютекс много весит, но пусть пока будет так:

__Решение #3:__
```cpp
pair<int32_t, mutex> accs[...];

void transfer(size_t from, size_t to, int32_t amount) {
  std::lock_guard lg1(accs[to].second);
  std::lock_guard lg2(accs[from].second);

  if (accs[from].first < amount)
    throw runtime_exception("...");
  accs[from].first -= amount;
  accs[to].first += amount;
}
```
* Появилась проблема _deadlock_:

| Thread 1 | Thread 2 |
| - | - |
| m[0].lock | |
| | m[1].lock |
| m[1].lock (wait...) | |
| | m[0].lock (wait...) |

__Решение #4:__

Оно основано на идее. Рассмотрим граф. В нем вершины - мьютексы. Есть ребро AB, если существует поток, который пытается заблокировать сначала A потом B.

Если в рассмотренном графе есть циклы, то возможен _dedlock_. Если ориентируем ребра от меньшего индекса вершины к большему, то циклов не будет.
```cpp
pair<int32_t, mutex> accs[...];

void transfer(size_t from, size_t to, int32_t amount) {
  std::lock_guard lg1(accs[min(to, from)].second);
  std::lock_guard lg2(accs[max(to, from)].second);

  if (accs[from].first < amount)
    throw runtime_exception("...");
  accs[from].first -= amount;
  accs[to].first += amount;
}
```

__Решение #5:__

```cpp
pair<int32_t, mutex> accs[...];

void transfer(size_t from, size_t to, int32_t amount) {
  std::unique_lock ul1(accs[to].second, std::defer_lock);
  std::unique_lock ul2(accs[from].second, std::defer_lock);

  std::lock(ul1, ul2); // no-deadlock-magic

  if (accs[from].first < amount)
    throw runtime_exception("...");
  accs[from].first -= amount;
  accs[to].first += amount;
}
```

__Решение #6:__
`std::scope_guard` - `lock_guard`, подднрживающий несколько mutex'ов.

```cpp
pair<int32_t, mutex> accs[...];

void transfer(size_t from, size_t to, int32_t amount) {
  std::scope_guard sg(accs[to].second, accs[from].second);

  if (accs[from].first < amount)
    throw runtime_exception("...");
  accs[from].first -= amount;
  accs[to].first += amount;
}
```
Оптимальнее было бы сделать один мьютекс на несколько счетов, например на 100.

`std::reqursive_mutex` - работает как обычный, только в рамках одного потока его можно заблокировать несколько раз. Соотвтственно столько же раз его надо разблокировать.

### Conditional variable
Нужен когда потоку чтобы исполниться нужно дождаться некоторого условия, зависящего от других потоков.

`cv.notify()` - сообщает какому-то потоку что навреное условие выполнено, можно попробовать исполниться.
`cv.notify_all()` - сообщает всем потокам об этом.

Например, когда producer завершил работу, можно написать более глобальное `cv.notify_all()`. В остальных случаях лучше потоки будить с помощью `cv.notify_one()` дабы не вредить производительности.

`cv.wait(ug)` - атомарно: разблокировать мьютекс, начать ожиание, потом заблокировать мьтекс.

```cpp
std::mutex m;
std::conditional_variablle cv;

std::unique_lock ul(m);
сv.wait(ul); // ждет пока другой поток не вызовет:
cv.notify_all(); // дороже, чем:
cv.notify_one();
```

```cpp
cv.wait(ul, [this](){
  return !q.empty(); // когда следует просыпаться
})
```


### Concurrent queue
Напишем pop (сразу идейно нормально, пояснения ниже)
```cpp
void push(T e) {
  // ...
  cv.notify_one();
}

T pop() {
  std::unique_guard ug(m);

  cv_empty.wait(ug, [this]{
    return !q.empty();
  });

  cv_full.notify_one(); // Оптимальнее:
  // говорим push, что очередь больше не заполнена (договариваемся, что есть некоторый наибольший размер)
  // ...
}
```
* Из очереди можно возвращать `std::optional<T>`, Но тогда пользователь скорее всего будет в цикле ждать элемент. Поэтому решаем сделать это ожидание за него.
* Нужно уметь засыпать и просыпаться для оптимальности. 
* _Thundering herd problem_ - много потоков ждут одного ресурса, но обработать его может только один, поэтому остальные продолжают ждать.
* Ошибка в коде ниже (это примерно принцип работы `conditional_variable` - то что внутри whilr):
```cpp
while (q.empty()) {
  ug.unlock();
  // здесь возможен push()
  cv.wait(); // псевдокод
  ug.lock();
}
```

## thread cancellaion
Есть потребность принудительно завершать потоки.

Самый простой способ плохой, все форсированные завершения небезопасны.

У `std::jthread`:
```cpp
std::jthread th([](std::stop_token const& st){
  while (!st.stop_requested()) { ... }
})

th.request_stop();
```

Более подробно (через более низкоуровневый `std::thread`):
```cpp
std::atomic<bool> stop_requested = false;
std::thread th([&stop_requested](){
  while (!stop_requested) { ... }
})

stop_request.store(true);
```

Нормальнее (почти в точности переписанный `jthread`, без учета RAII):
```cpp
std::stop_source stop_src;
std::thread th([&stop_src](){
  while (!stop_src.stop_requested()) { ... }
})

stop_src.request_stop();
th.join();
```

Проблема: в потоке большой стек вызова и явно не хочется передавать через много функций `stop_token`.

Решение: использовать исключения (скажем, `throw interrupt();`). Тут все равно надо пропагейтить `stop_token`, но как идея, это возможно поможет.

## Асинхронные операции
Идейно понятно.

_polling_ - антипаттерн, который может помочь справиться с блокирующими вызовами.

## Блокирующие вызовы
... - вызов ждущий события.

Примеры:
* `read()`, `write()` (`<io.h>`)
* `recv`/`send` в сети
* `sleep()`
* pthread_mutex_lock, pthread_cond_wait, pthread_join - какие-то сишные штуки.

Чтобы сделать _cancellation_ при блокирующем вызове
