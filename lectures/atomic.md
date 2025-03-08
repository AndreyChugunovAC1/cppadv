# Atomic (std::atomic)

Делает операции с хранимым типом атомарными - неделимыми, то есть другие потоки не смогут подгадить эти операции.

```cpp
std::atomic<int32_t> a;

{
  int32_t b = a;
  int32_t c = a.load();

  a = 1;
  a.store(1);

  a += 2;
  a.fetch_add(2);

  int32_t old_value = a.exchange(3);
}
```

Показанные операции атоманые.

__Решение #7 (задача в multithreading.md):__
```cpp
std::atomic<int32_t> accs[...];

void transfer(size_t from, size_t to, int32_t amount) {
  if (accs[from].first < amount)
    throw runtime_exception("...");
  accs[from].first -= amount;
  accs[to].first += amount;
}
```
* Это конечно не решение, тут нужно еще аккуратно написать `compare_exchange_weak`. Но суть в том, что можно просто использовать `atomic`.

## mutex через atomic
Самый содержательный вопрос, что писать в цикле ожидания (также атомарная замена состояния - здесь она написана):
```cpp
void lock() {
  while (state.exchange(locked) == locked) {
    // (1) mirgates thread:
    //     std::this_thread::yield();
    //     sched_yield();
    //     sleep(0)

    // (2) sleep(???);

    // (3) <empty>
  }
}
```
1) Есть тонкая причина, которую не понял даже человек, взявшийся это исследовать (идея: мигрировать потоки с ядра на ядро - дорого из-за кэшей)
2) Если ждать какое-то время, то появляется заметный лаг в программе - это неприемлимо

__Другая идея:__
```cpp
void lock() {
  while (state.exchange(locked) == locked) {
    os_specific_wait();
  }
}

void unlock() {
  state.store(unlocked).
  os_specific_wake();
}
```
Хочется чтобы `unlock` будил именно того кто спит в этом `mutex`.

Для этого в линуксе есть _futex_ - он умеет ждать "на адресе". wake - будет всех кто ждет на адресе. Ксли кратко, то:
```cpp
// примерно так
futex::wait( address , expected_value );
futex::wake( address ); 

// usage
futex::wait(state, locked);
..
futex::wake(state);
```
На самом деле сигнатура в разы ужаснее.

У `atomic` есть почти знакомые нам методы:
```cpp
av.wait(expected_value);
av.notify_one();
av.notify_all();

// неплохая версия mutex
void lock() {
  while (state.exchange(locked) == locked) {
    state.wait(locked);
  }
}

void unlock() {
  state.store(unlocked).
  state.notify_one();
}
```

Когда `mutex` блокируют редко либо и вовсе работает только один поток, хочется чтобы все эти вызовы были дешевле. То есть делать `notify_one` только если кто-то ждет.

__Решение #8 (про compare_exchange):__

Также пример типового цикла с `compare_exchange`:

```cpp
auto old = accs[from].load();

for(;;) {
  if (old < amount)
    throw ...
  if (accs[from].compare_exchange_weak(old, old - amount))
    break;
}
// ...
```

CAS (compare-and-swap/compare_exchange_weak) раотает так:
```cpp
a.compare_exchange_weak((int &)expected, (int)desired);
```
Все атомарно: сравнивает `a` с `expected`. Если они равны, то в `a` записывается `desired`, возвращается true. Если нет, то в `expected` записывается `a`, возвращается false.

`compare_exchange_strong` - это `compare_exchange_weak` но в цикле. Weak может ошибиться, strong как бы нет.

_Three-state-mutex_ - мьютекс с оптимизацией: будет только если есть кого будить:
```cpp
enum class ms { // mutex state
  locked_no_waiters,
  locked_potential_waiters,
  unlocked
};

void lock() {
  ms expected = ms::unlocked;

  if (state.compare_exchange_strong(expected, ms::locked_no_waiters))
    return;
  while (state.exchange(ms::locked_potential_waiters) != ms::unlocked) {
    state.wait(ms::locked_potential_waiters);
  }
}

void unlock() {
  ms old = state.exchange(ms::unlocked);

  if (old == ms::locked_potential_waiters)
    state.notify_one();
}
```
Факты про атомики:
* На самом деле атомики сначала пытаются делать _busy-wait_ в цикле (пустой while) - это лучше, если надо ждать дешевой операции. Если он крутится долго, то все таки отправляется ожидать.  Это гибридный подход.
* _spin-lock_ - мьютекс, который ждет крутясь в пустом цикле.

## relaxed атомики
Проблема: процессор устроен даже сложнее чем и без того непрстая модель вычислителя и RAM. Поэтому если использовать просто `int`, то да, вроде бы никаких проблем чтобы операции работали независимо (есть же кэш когерентность). Но на самом деле, гарантии, что инструкции будут производиться в предсказуемом порядке, нет. С целью оптимизации архитектуры иногда переупорядочивают операции. 

То есть: инструкции сами по себе атомарные, но этого мало, поэтому у всех методов атомика есть доп. параметр `std::memory_order`, который делает _барьеры памяти_.

Есть курьезный пример, когда по код теоретически точно делает либо (x = 1, y = 0), либо (y = 0, x = 1), а в конце получается что оба 0, что мягко говоря странно.
```cpp
std::atomic<int> x, y;

void thread_1() {
  x.store(1);
}

void thread_2() {
  y.store(1);
}

void thread_3() {
  int x3 = x.load();
  int y3 = y.load();
  if (x3 == 1 && y3 == 0) {
    std::cout << "x written before y\n";
  }
}

void thread_4() {
  int y4 = y.load();
  int x4 = x.load();
  if (y4 == 1 && x4 == 0) {
    std::cout << "y written before x\n";
  }
}
```

В коде выше этот пример реализуется так: на экран может не быть выведено ничего.

* `std::memory_otder_release` означает, что все операции, сделаные до него, не могут быть выполнены после в результате переупорядочивания.
* `std::memory_order_acquire` означает, что все операции после него не могут быть выполнены до.
* Обычно release-aquire идут в паре:
```cpp
a.store(value, std::memory_order_release);
a.load(std::memory_order_acquire);
```
* `std::memory_order_relaxed` - значение по умолчанию. Не накладывает ограничений на переупорядочивание операций.

_sequential consistency_ - операции i/o идут в том порядке, в котором они написаны.

_Эфект sharing_ - много потоков пишут в одну переменную (это долго).

## volatile (keyword)

В первую очередь стоит сказать, что `volatile` был связян с многопоточностью в Си и в C++ до C++11, но сейчас __никак не связян__ с многопоточностью в C++ (по крайней мере напрямую).

__Для чего нужен `volatile`:__
* В __java__ `volatile` - аналог атомиков.
* `volatile` (как и `atomic`) подавляет оптимизации компилятора, но они дают разные гарантии.
* device memory - работа с памятью внешних устройств без каких бы то ни было оптимизаций.
* Используется как-то с setjmp/longjmp - это более аккуратная версия goto. "Нет уважительных причин их использовать".
* UNIX-сигналы: если внешний обработчик сигнала работает с переменными, то они должны быть `volatile`.

Вывод состоит примерно в том, что `volatile` не следует использовать вовсе.

## thread_local
... - клюлчевое слово, которое говорит: для каждого потока есть свой экземпляр. То есть при создании потока будет инициализирована такая переменная.

Например, сишный `rand()` использует `thread_local seed`. Поэтому он на самом деле _thread-safe_, но не особо полезен все равно. Следует использовать C++-ные генераторы случайных чисел.

Хороший тон сделать RAII обертку над потоком:
```cpp
struct mythread {
  mydata thread_data;
  jthread thread;
}
```

