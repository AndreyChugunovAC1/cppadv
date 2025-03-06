#include <memory>
#include <functional>

int main() {
  std::unique_ptr<int> up(new int(3));

  auto /* better not std::function<void(int*)> */ deleter = [](int *p) {
    delete p;
  };

  std::unique_ptr<int, decltype(deleter)> up(new int(3), deleter);

  std::shared_ptr<int> sp(new int(3), deleter);
}