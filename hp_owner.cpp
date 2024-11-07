#include <atomic>
#include <functional>
#include <stdexcept>
#include <thread>

using std::runtime_error;

constexpr unsigned max_hazard_pointers = 100;

struct hazard_pointer {
  std::atomic<std::thread::id> thread_id;
  std::atomic<void *> pointer1;
};

hazard_pointer hazard_pointers[max_hazard_pointers];

class hp_owner {
public:
  hp_owner(hp_owner &&) = default;
  hp_owner &operator=(hp_owner &&) = default;

  ~hp_owner() {
    hp->id.store(std::thread::id{});
    hp->pointer1.store(nullptr);
  }

  hp_owner() : hp(nullptr) {
    for (unsigned i = 0; i < max_hazard_pointers; ++i) {
      std::thread::id default_id{};
      if (hazard_pointers[i].thread_id.compare_exchange_strong(
              default_id, std::this_thread::get_id())) {
        hp = &hazard_pointers[i];
        break;
      }
    }
    if (!hp) {
      throw runtime_error("No hazard pointers available!");
    }
  }

  std::atomic<void *> &get_pointer1() { return hp->pointer1; }

private:
  hazard_pointer *hp; // no owning
private:
  hp_owner(const hp_owner &) = delete;
  hp_owner &operator=(const hp_owner &) = delete;
};

std::atomic<void *> &get_hazard_pointer_for_current_thread() {
  thread_local static hp_owner hp {};
  return hp.get_pointer1();
}

