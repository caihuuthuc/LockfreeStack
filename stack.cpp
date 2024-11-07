#include "hp_owner.cpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

namespace my_impl {
template <typename T> class lockfree_stack {
public:
  struct Node {
    std::shared_ptr<T> data;
    Node(const T &d) : data(std::make_shared<T>(d)), next(nullptr) {
      std::cout << "Node::Node(const T&)\n";
    }
    Node(T &&d) : data(std::make_shared<T>(std::move(d))), next(nullptr) {
      std::cout << "Node::Node(T&&)\n";
    }
    Node *next;
  };

  lockfree_stack() noexcept : head{nullptr} {};
  lockfree_stack(lockfree_stack &&) = default;
  lockfree_stack &operator=(lockfree_stack &&) = default;

  std::shared_ptr<T> pop() {
    // pop the head
    // head->next become new head

    // get hp of this thread
    std::atomic<void *> &hp = get_hazard_pointer_for_current_thread();
    Node *old_head = head.load(); // pop this head
    do {
      // try to store current head in hp
      Node *temp;
      do {
        temp = old_head;
        hp.store(old_head);
        old_head = head.load();
      } while (old_head !=
               temp); // if another thread owned this head, get the new head
    } while (old_head &&
             !head.compare_exchange_strong(old_head, old_head->next));
    // if the old_head is the current head of the stack, promote old_head->next
    // become new head of the stack
    // -----------------------------------------------
    // at this time, this thread is owning the old_head
    // no longer need the hp
    hp.store(nullptr);
    std::shared_ptr<T> res;
    if (old_head) {
      res.swap(old_head->data);
      std::cout << *res << "\n";
      delete old_head;
    }
    return res;
  }

  void push(T d) {
    Node *new_node = new Node{std::move(d)};
    new_node->next = head.load();
    while (!head.compare_exchange_weak(new_node->next, new_node))
      ; // no op
  }

private:
  std::atomic<Node *> head;

private:
  lockfree_stack(const lockfree_stack &) = delete;
  lockfree_stack &operator=(const lockfree_stack &) = delete;
};
}; // namespace my_impl
