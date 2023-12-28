# EventManager
When registering a coroutine, you must ensure the coroutine object lives until the end of the execution of the coroutine, and it is your responsibility to ensure this is the case.

i.e if you register a coroutine from an external connection, then you could spawn a coroutine but also pass a lambda which does the post processing (i.e removed the EvTask corresponding to it from whatever data structure you're using to manage incoming connections).

## Queue Submission
You can make a queue with `EventManager::make_request_queue` and use those methods to effectively queue up a bunch of operations, and then you can submit them all at once, and set a callback for processing them using `EventManager::submit_and_wait`.

# EvTask
- You can use it for coroutines, and await on awaitable objects in a function with this return type
- You can await on it, and it will return an integer
- If you await on it, then the current coroutine will be suspended if the inner coroutine is suspended
  - This leads to a semblance of sequential execution for a stack of coroutine calls

# Adding More Operations
To add more operations, there are a few files you need to update:
## In communication/
1. Add a new enum request type representing the request in `communiation_types.hpp`
2. Add a new data type which would store the response data you want in `response_packs.hpp`
3. Add a new type trait specialisation for the enum value (i.e a new mapping from the enum to the data type) in `communication_types.hpp`
4. Add `RespDataTypeMap<RequestType::your_new_request_enum>` to the variant at the bottom (before the monostate) of `communication_types.hpp`
## In event_loop/parameter_packs.hpp
1. Make a struct containing the parameters needed for your request, i.e for `readv`:
```cpp
struct ReadvParameterPack {
  int fd;
  struct iovec *iovs;
  size_t num;
};
```
2. Add that new parameter pack to the `OperationParameterPackVariant`
3. Add a new type trait specialisation for the enum value you added in `communication_types.hpp`, like this:
```cpp
template <> struct RequestToParamPack<RequestType::READV> {
  using type = ReadvParameterPack;
};
```
4. And then define a new operation on the `RequestQueue` for your operation, like this:
```cpp
void queue_readv(int fd, struct iovec *iovs, size_t num) {
  req_vec.push_back(ReadvParameterPack{fd, iovs, num});
}
```
## In event_loop/request_data.hpp
Add another entry to the `specific_data` union, like:
```cpp
  ...
  ReadvParameterPack readv_data;
  ...
```
## In coroutine/io_awaitables.hpp
You need to add a new awaitable for your request of the form below in this file:
```cpp
struct [/* operation name */]Awaitable : IOAwaitable<RequestType::[/* operation name */], [/* operation name */]Awaitable> {
  void prepare_sqring_op(EvTask::Handle handle, io_uring_sqe *sqe) {
    auto &[/* operation name */]_data = req_data.specific_data.[/* operation name */]_data;
    io_uring_prep_[/* operation name */](sqe, ...);
  }

  [/* operation name */]Awaitable(int fd, struct iovec *iovs, size_t num, EventManager *ev)
      : IOAwaitable(ev) {
    auto &[/* operation name */]_data = req_data.specific_data.[/* operation name */]_data;
    [/* operation name */]_data = {fd, iovs, num};
  }

  // default initialiser
  [/* operation name */]Awaitable() : IOAwaitable(nullptr) {}
};
```
This will allow us to later do stuff like `co_await ev->[/* operation name */](...)`

### In event_manager.hpp
1. Firstly add a forward declaration for the awaitable you made above
2. 
## In event_loop/event_manager.hpp
1. Add a forward declaration for the awaitable you made above, we can't include it here since that file includes this file too
2. Add an appropriate declaration to make use of the awaitable in the event manager, like this:
```cpp
ReadvAwaitable readv(int fd, struct iovec *iovs, size_t num);
```
## In event_loop/io_ops.cpp
1. Define the operation you declared above like this:
```cpp
ReadvAwaitable EventManager::readv(int fd, struct iovec *iovs, size_t num) {
  if (should_restrict_usage())
    return {};
  return ReadvAwaitable{fd, iovs, num, this};
}
```
2. Add another case to the switch case in the `submit_and_wait(...)` function corresponding to this operation, i.e:
```cpp
case RequestType::READV: {
  auto *pack = std::get_if<ReadvParameterPack>(&req);
  if (pack) {
    specific_data.readv_data = {pack->fd, pack->iovs, pack->num};
    io_uring_prep_readv(sqe, pack->fd, pack->iovs, pack->num, 0);
  } else {
    std::cerr << "There was an error in retrieving queued data\n";
    co_return -1;
  }
  break;
}
```
## In event_loop/core.cpp
Add in a case for your operation in the `event_hanlder(...)` switch case, like this:
```cpp
case RequestType::READV: {
  ReadvResponsePack data{};
  if (res < 0) {
    data.error_num = errno;
  } else {
    data = {.bytes_read = res, .buff = specific_data.read_data.buffer};
  }
  data.fd = specific_data.readv_data.fd;
  promise.publish_resp_data<RequestType::READV>(std::move(data));
  req_data->handle.resume();
  break;
}
```

# Todo
1. Add in these operations:
  - `openat`
  - `statx`
  - `unlinkat`
  - `renameat`