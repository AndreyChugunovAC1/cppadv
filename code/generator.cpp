#include <coroutine>
#include <iostream>
#include <optional>

// Генератор, который возвращает значения типа T
template <typename T>
struct Generator {
  struct promise_type {
    T current_value; // Текущее значение

    // Возвращает объект генератора
    Generator get_return_object() {
      return Generator{
        std::coroutine_handle<promise_type>::from_promise(*this)
      };
    }

    // Корутина приостанавливается при старте
    std::suspend_always initial_suspend() { return {}; }

    // Корутина приостанавливается при завершении
    std::suspend_always final_suspend() noexcept { return {}; }

    // Возвращает значение и приостанавливает корутину
    std::suspend_always yield_value(T value) {
      current_value = value;
      return {};
    }

    // Завершает корутину
    void return_void() {}

    // Обрабатывает исключения
    void unhandled_exception() { std::terminate(); }
  };

  // Тип handle для управления корутиной
  using handle_type = std::coroutine_handle<promise_type>;

  // Конструктор
  explicit Generator(handle_type handle) : handle_(handle) {}

  // Деструктор
  ~Generator() {
    if (handle_) {
      handle_.destroy();
    }
  }

  // Оператор перемещения
  Generator(Generator &&other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }

  // Оператор присваивания перемещением
  Generator &operator=(Generator &&other) noexcept {
    if (this != &other) {
      if (handle_) {
        handle_.destroy();
      }
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  // Получить следующее значение
  std::optional<T> next() {
    if (!handle_ || handle_.done()) {
      return std::nullopt; // Корутина завершена
    }
    handle_.resume();                       // Возобновляем корутину
    return handle_.promise().current_value; // Возвращаем текущее значение
  }

private:
  handle_type handle_; // Управляющий объект корутины
};

// Пример корутины-генератора
Generator<int> generateNumbers(int n) {
  for (int i = 0; i < n; ++i) {
    co_yield i * i; // Возвращаем значение и приостанавливаемся
  }
}

int main() {
  auto gen = generateNumbers(5); // Создаём генератор

  while (auto value = gen.next()) {
    std::cout << "Generated: " << *value << "\n";
  }

  std::cout << "Generator finished.\n";
  return 0;
}