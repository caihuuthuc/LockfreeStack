#include "stack.cpp"
#include <thread>

int main() {
  my_impl::lockfree_stack<int> stack;
  std::thread t1([&]() { stack.push(5); });
  std::thread t2([&]() { stack.push(13); });

  t1.join();
  t2.join();

  std::thread t3([&]() { auto s = stack.pop(); });
  std::thread t4([&]() { auto s = stack.pop(); });

  t3.join();
  t4.join();
}