#include <iostream>
#include <mutex>
#include <thread>

void example_thread() {
  constexpr std::size_t THREAD_COUNT = 30;

  std::thread ths[THREAD_COUNT];
  int arr[THREAD_COUNT] = {};

  for (std::size_t i = 0; i < THREAD_COUNT; i++) {
    ths[i] = std::thread([&arr, idx = i](std::stop_token const &st) {
      // st.stop_requested();
      for (int i = 0; i < 10000000; i++) {
        arr[idx] += 1;
      }
    });
  }

  for (auto &th : ths) {
    th.join();
  }

  for (int x : arr) {
    std::cout << x << " ";
  }
  std::cout << std::endl;
}

void example_jthread() {
  constexpr std::size_t THREAD_COUNT = 30;
  int arr[THREAD_COUNT] = {};

  {
    std::jthread ths[THREAD_COUNT];

    for (std::size_t i = 0; i < THREAD_COUNT; i++) {
      ths[i] = std::jthread([&arr, idx = i] {
        for (int i = 0; i < 10000000; i++) {
          arr[idx] += 1;
        }
      });
    }
  }

  for (int x : arr) {
    std::cout << x << " ";
  }
  std::cout << std::endl;
}

void example_mutex() {
  constexpr std::size_t THREAD_COUNT = 30;
  int value = 0;
  std::cout << "NO MUTEX\n";
  {
    std::jthread ths[THREAD_COUNT];

    for (std::size_t i = 0; i < THREAD_COUNT; i++) {
      ths[i] = std::jthread([&value, idx = i] {
        for (int i = 0; i < 10000000; i++) {
          value += 2 * (i % 2) - 1;
        }
      });
    }
  }
  // random value (usually I get ~250)
  std::cout << "value: " << value << "\n";
  value = 0;
  std::cout << "WITH MUTEX\n";
  {
    std::mutex m;
    std::jthread ths[THREAD_COUNT];

    for (std::size_t i = 0; i < THREAD_COUNT; i++) {
      ths[i] = std::jthread([&m, &value, idx = i] {
        for (int i = 0; i < 10000000; i++) {
          m.lock(); // not good
          value += 2 * (i % 2) - 1;
          m.unlock();
        }
      });
    }
  }
  // always 0:
  std::cout << "value: " << value << "\n";
}

void example_mutex_lock_guard() {
  constexpr std::size_t THREAD_COUNT = 30;
  int value = 0;
  {
    std::mutex m;
    std::jthread ths[THREAD_COUNT];

    for (std::size_t i = 0; i < THREAD_COUNT; i++) {
      ths[i] = std::jthread([&m, &value, idx = i] {
        for (int i = 0; i < 10000000; i++) {
          std::lock_guard lg(m);

          value += 2 * (i % 2) - 1;
        }
      });
    }
  }
  // always 0:
  std::cout << "value: " << value << "\n";
}

int main() {
  // example_thread();
  // example_jthread();
  // example_mutex();
  example_mutex_lock_guard();
}