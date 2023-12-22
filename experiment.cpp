#include "communication/communication_channel.hpp"
#include "communication/response_types.hpp"
#include "coroutine/awaiter.hpp"
#include "coroutine/task.hpp"
#include <chrono>
#include <coroutine>
#include <iostream>
#include <liburing.h>
#include <mutex>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

constexpr const int QUEUE_DEPTH = 10;

struct Job {
  EvTask coro;
  CommunicationChannel *channel{};

  Job(EvTask &&coroutine, CommunicationChannel *com_channel)
      : coro(std::move(coroutine)), channel(com_channel) {}
};

class EventManager {
  static std::mutex init_mutex;
  static int shared_ring_fd;
  static int ring_instances;

  io_uring ring{};

  std::vector<EvTask> coroutines_to_start{};
  std::vector<Job> jobs{};
  // enum St
public:
  EvTask kill_ring() {
    // co_await read on kill pfd

    /* i.e submit kill requests for all pfds - co_await on the submission bit */

    // for (size_t i = 0; i < pfd_to_data.size(); i++) {
    //   if (!pfd_freed_pfds.contains(i)) {
    //     queue_cancel_request_by_pfd(i);

    //     end_stage_num_to_cancel += pfd_to_data[i].submitted_reqs;
    //   }

    //   // don't queue more than can be fit in the queue
    //   if (current_num_of_queued_sqes >= QUEUE_DEPTH) {
    //     submit_all_queued_sqes_privately();
    //   }
    // }
    // // submit any which haven't been submitted yet
    // submit_all_queued_sqes_privately();

    // // not dead but not just dying
    // if (end_stage_num_to_cancel != 0) {
    //   manager_life_state = living_state::DYING_STAGE_2_CANCELLING_REQS;
    // } else {
    //   manager_life_state = living_state::DEAD; // nothing left to cancel,
    //   we're done
    // }

    /* This bit isn't necessary anymore since we co_await killing all tasks */

    // end_stage_num_to_cancel--;

    // if (end_stage_num_to_cancel == 0) {
    //   manager_life_state = living_state::DEAD;
    // }

    /* This finally kills the uring, and should invoke some cleanup function at
     * the end */

    // // if it is dead, then this is the final iteration of the event loop,
    // // so kill the ring
    // io_uring_queue_exit(&ring);
    // stdlib_close(pfd_to_data[kill_pfd].fd);

    // ring_instances--; // this instance of the ring is now dead

    // // this is the only callback that could be tried to be called even if
    // they weren't set callbacks->killed_callback(); // event_manager has been
    // killed, call the last callback
  }

  void register_coro(EvTask &&coro) {
    coroutines_to_start.emplace_back(std::move(coro));
  }

  void register_coro(EvTask &coro) { coroutines_to_start.push_back(coro); }

  EventManager() {
    std::scoped_lock<std::mutex> lock{init_mutex};

    if (shared_ring_fd == -1 ||
        ring_instances ==
            0) { // uses a shared asynchronous backend for all threads
      io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
      shared_ring_fd = ring.ring_fd;
    } else {
      io_uring_params params{};
      params.wq_fd = shared_ring_fd;
      params.flags = IORING_SETUP_ATTACH_WQ;
      io_uring_queue_init_params(QUEUE_DEPTH, &ring, &params);
    }

    register_coro(kill_ring());

    // move kill_coro, com_channel ptr -> visitor for processing
    // visitor then submit io req by moving kill_coro to dyn alloc req

    ring_instances++;
  }

  void start() {
    while (true) { // replace with while alive
      // start all queued up coroutines, then empty the vector
      for (auto &coro : coroutines_to_start) {
        auto channel = coro.start();
        jobs.emplace_back(std::move(coro), channel);
      }

      coroutines_to_start.clear();

      // process all jobs then empty the vector
      for (auto &job : jobs) {
        process_job(std::move(job));
      }

      jobs.clear();

      await_message();
    }
  }

  void await_message() {}

  void process_job(Job &&job) {
    // should call stuff like io_uring_submit
  }

  int uring_submit() {
    // returns either number of submitted
  }

  void submit_queued_operations() {
    //
  }

  // need way to kill event loop
  // need way to register coroutine with event manager
};

struct GenericResponse {
  ResponseType resp_type{};
  const ResponseVariant *response_data{};
};

struct PollResponses {
  CommunicationChannel *channel = nullptr;

  void await_suspend(typename EvTask::Handle handle) {
    channel = &handle.promise().state.com_data;
  }

  GenericResponse await_resume() {
    /*
    in coro we call co_await PollResponses{...}
    then in event loop:
    ev_task->resume<ResponseType>(resp_data) is called
    and so then this await_resume is called
    we know then exactly one response is available
    so stored_data.response_store_ has one item
    */
    auto type = channel->response_store_current_type();
    const ResponseVariant *resp_store = &channel->response_store_;
    return {type, resp_store};
  }
};

EvTask submit_and_wait() {
  // co_await submit_requests()
}

void visit_request_types(CommunicationChannel *channel) {
  if (!channel) {
    std::cerr << "Cannot visit types in channel since it's a nullptr\n";
    return;
  }

  switch (channel->request_store_current_type()) {
  case RequestType::READ: {
    std::string str = "it was a read request: ";
    str +=
        std::to_string((int)channel->get_req_data<RequestType::READ>().value());
    std::cout << str << "\n";
    break;
  }
  case RequestType::WRITE: {
    std::string str = "it was a write request: ";
    str += std::to_string(
        (int)channel->get_req_data<RequestType::WRITE>().value());
    std::cout << str << "\n";
    break;
  }
  case RequestType::OPEN:
  case RequestType::CLOSE:
  case RequestType::SHUTDOWN:
  case RequestType::READV:
  case RequestType::WRITEV:
  case RequestType::ACCEPT:
  case RequestType::CONNECT:
    break;
  }
}

EvTask coro2() {
  std::cout << "in coro2 before sleep\n";
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::cout << "after coro2 sleep\n";
  co_return;
}

EvTask coro1() {
  std::cout << "before coro2\n";
  co_await coro2();
  std::cout << "after coro2\n";
}

int EventManager::shared_ring_fd = -1;
int EventManager::ring_instances{};
std::mutex EventManager::init_mutex{};

int main() {
  auto c = coro1();
  c.start();
  std::cout << "Hello world\n";
  std::this_thread::sleep_for(std::chrono::seconds(3));
}

/*

####
For later:
####

in coro function:
```
auto read_data = co_await read(...) // this will be a queue, submit and polling
at once

// each co_await queue should increment handle.state.queued by 1
co_await queue_read(...)
co_await queue_write(...)
co_await queue_connect(...)

// this should decrement handle.state.queued by however many were submitted
// and set handle.state.submitted by the return code of io_uring_submit
// return tuple (num_submitted ptr, num_still_queued ptr)
// so user can deal with resubmission
(num_submitted, num_still_queued) = co_await submit_requests()

// assuming those 2 are valid pointers
while(*num_submitted != 0 || *num_still_queued != 0) {
  // this inner loop simply retries io_uring_submit until all queue items are
submitted eventually
  // if even one item is submitted, we leave and try to reap it instead
  while(*num_submitted == 0 && *num_still_queued != 0) { // if all items are
still queued at this stage co_await submit_requests(); // this should mutate
num_submitted if more are submitted
  }

  // reap the response
  auto job_response = co_await poll_responses()

  // response_data is std::variant<response_type 1, response_type 2 ...
response_type N>* auto [response_type, response_data] = job_response;
  switch(response_type) {
    case RESPONSE_TYPE::READ:
      // handle read response logic
      break;
    case RESPONSE_TYPE::WRITE:
      // handle write response logic
    ...
  }
}

// by this point all queued items should have been dealt with
```

in queue_read::await_suspend(Handle h) for example:
```
...
auto ret = io_uring_get_sqe()

if(!ret) {
  // pass back to coro ret
  return
}

h.state.queued += 1

// pass back to coro ret
```

in submit_requests::await_suspend(Handle h):
```
...
int ret = io_uring_submit()
if(ret > 0) {
  h.state.queued -= ret
  h.state.submitted += ret
}

// pass back to coro (&h.state.submitted, &h.state.queued)
```

in poll_responses::await_suspend(Handle h):
```
// wait for event loop completion event
```

queued multiple things
submitted all N

1 of them returns
you call resume with this, resume knows you are waiting for multiple so adds to
a vector N-1 more show

*/