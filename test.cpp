#include <atomic>
#include <coroutine>
#include <iostream>
#include <thread>
#include <vector>

/*
The idea of this snippet is as such:
> coro1 does some stuff then co_await coro2
> coro2 does some more stuff and co_await coro3
> coro3 does some other stuff and suspends,
> submitting its handle via an awaitable to another thread
> that other thread does some stuff then resumes coro3
> coro3 does some stuff and then suspends again, it does this
> process again and again 5 times
> coro3 then does some final work and co_return
> coro2 does same as above
> coro1 does some as above

This is a sample execution:
> got to coro1 (thread id: 140494083028800)
> got to coro2 (thread id: 140494083028800)
> got to coro3 (thread id: 140494083028800)
> suspending coro3
> resuming coro3
> suspending coro3
> resuming coro3
> suspending coro3
> resuming coro3
> suspending coro3
> resuming coro3
> suspending coro3
> resuming coro3
> at end of coro3 (thread id: 140494076114496)
> at end of coro2 (thread id: 140494076114496)
> at end of coro1 (thread id: 140494076114496)

Note how since we resumed coro3 on another thread, the
rest of the execution happens on that other thread.
*/

bool coro1_finished = false;
std::atomic<bool> coro3_suspended{};
std::atomic<std::coroutine_handle<>> coro3_handle{};
std::atomic<bool> coro3_repeating_procedure{};

void print_message_with_tid(const std::string &msg) {
  std::cout << msg << " (thread id: " << std::this_thread::get_id() << ")\n";
}

struct Task {
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    struct {
      std::exception_ptr exception_ptr{};
      // awaiter_handles is the important bit
      // this stores all the handles of coroutines that have called co_await
      // on this coroutine
      // then in final_suspend, those coroutines are resumed
      // this allows us to
      std::vector<std::coroutine_handle<>> awaiter_handles{};
    } state;

    Task get_return_object() { return Task{Handle::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept {
      for (auto &h : state.awaiter_handles) {
        h.resume();
      }
      return {};
    }

    void return_void() {}

    void unhandled_exception() {
      state.exception_ptr = std::current_exception();
    }
  };

  Handle handle{};

  Task(Handle h) : handle(h) {}

  void resume() {
    auto &state = handle.promise().state;
    handle.resume();

    if (state.exception_ptr) {
      std::rethrow_exception(state.exception_ptr);
    }
  }

  bool is_done() { return handle.done(); }

  explicit operator bool() { return is_done(); }

  bool await_ready() const noexcept { return false; };
  void await_suspend(typename Task::Handle other_handle) {
    // verified that the handle here and the stored handle are indeed different
    // std::cout << this->handle.address() << " ## " << other_handle.address()
    // << "\n"; the other_handle is the handle of the coroutine calling await on
    // this coroutine (the coroutine corresponding to this->handle)

    auto &state = this->handle.promise().state;
    auto &awaiter_handles = state.awaiter_handles;
    awaiter_handles.push_back(other_handle);
  }
  void await_resume() {}
};

struct SetAtomicAwaitableForCoro3 {
  bool await_ready() const noexcept { return false; }
  void await_suspend(typename Task::Handle h) { coro3_handle.store(h); }
  void await_resume() {}
};

Task coro3() {
  print_message_with_tid("got to coro3");

  int num_reps = 5;
  auto awaitable = SetAtomicAwaitableForCoro3{};
  coro3_repeating_procedure = true; // begun the procedure

  // repeat suspending and resuming num_reps times
  for (int i = 0; i < num_reps; i++) {
    coro3_suspended = true;
    std::cout << "suspending coro3\n";
    co_await awaitable;
  }

  coro3_repeating_procedure = false; // have finished the procedure

  print_message_with_tid("at end of coro3");
}

Task coro2() {
  print_message_with_tid("got to coro2");

  auto task3 = coro3();
  task3.resume();

  co_await task3;

  print_message_with_tid("at end of coro2");
}

Task coro1() {
  print_message_with_tid("got to coro1");

  auto task2 = coro2();
  task2.resume();

  co_await task2;

  print_message_with_tid("at end of coro1");

  coro1_finished = true;
}

void other_thread() {
  while (true) {
    // busy wait until we have both a handle that represents the coroutine
    // and also that the coroutine is suspended
    std::coroutine_handle<> local_handle{};
    while (!(local_handle = coro3_handle.load()) || !coro3_suspended) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ensures to only resume only if the coroutine is actually still suspended
    if (coro3_suspended) {
      coro3_suspended = false;
      std::cout << "resuming coro3\n";
      local_handle.resume();
    }

    // if we're no longer doing the repeating procedure, then we are done
    if (!coro3_repeating_procedure) {
      break;
    }
  }
}

int main() {
  auto task1 = coro1();
  task1.resume();

  // after coro3 suspends for the first time, this thread
  // continues execution and reaches here

  auto t = std::thread(other_thread);
  t.join();
}
