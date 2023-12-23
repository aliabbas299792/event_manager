# EvTask
- You can use it for coroutines, and await on awaitable objects in a function with this return type
- You can await on it, and it will return an integer
- If you await on it, then the current coroutine will be suspended if the inner coroutine is suspended
  - This leads to a semblance of sequential execution for a stack of coroutine calls

# Todo
0. Firstly address the comment at the top of `core.cpp`
1. Introduce the basic io_uring setup and event loop
2. There is only one asynchronous thing to do, `io_uring_submit` and then waiting for it
  - To test it first, add a read/write prep function on the event loop
  - Implement the `submit_and_wait(...)` method described below, which should make use of the awaiting on coroutines functionality I developed in the github gist
  - Implement all the `co_await op` stuff in terms of that
2. Rather than switching on some tag in the event loop, use the visitor pattern for the task itself
3. At the end of the event loop for each iteration
  1. Collect a new potential request object
  2. If it exists the visitor pattern to dispatch the correct io_uring job

For clarifying ideas, have a look at the long comment in `experiment.cpp`.

# Interface
You should be able to do `co_await queue_operation(...)` operations and also `co_await operation(...)`.
- The former should merely do `io_uring_get_sqe` and queue up the operation
- The latter should do that and:
  - call `io_uring_submit` over and over until the item is actually submitted
  - upon resumption from the event loop pass back the response data to the caller


There also needs to be a `submit_and_wait(...)` awaitable.
- This function should follow the looping method described in the comment in `experiment.cpp` to submit everything
- Internally it should call `co_await submit_requests(...)` which should facilitate the looping method above
- And it should call `co_await poll_responses()`, which should be resumed upon received any response for any request for this coroutine
- `submit_and_wait` should take a lambda `resp_handler`, we call `resp_handler(CommunicationChannel *cc)`, so the user is able to use `response_store_current_type()` and `get_resp_store_data<T>()` to dynamically retrieve any data

This looping method allows us to submit any number of events at once, and then effectively poll for them in our coroutine in a much more simple manner.

