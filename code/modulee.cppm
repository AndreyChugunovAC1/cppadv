export module modulee;

export {
  int diff(int a, int b);
}

// private function
int add(int a, int b) {
  return a + b;
}

// public function:
export int sum(int a, int b) {
  return add(a, b);
}

int diff(int a, int b) {
  return a - b;
}