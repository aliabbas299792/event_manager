# Event Manager
Simple liburing based library which uses callbacks for I/O events.

# How to use
## Setup
The `event_manager` runs asynchronously, so you'll need to provide callback functions to get use out of it.

This is done by inheriting from `server_methods` and overriding its default methods with whatever functionality you choose.

For example:
```cpp
my_callbacks callbacks{};

event_manager ev{};
ev.set_callbacks(&callbacks);
```

And then to run the event manager, just call `ev.start()`.

## Usage
Since `start()` will block the current thread, I'll run it in another thread below.

To use a file descriptor of some sort with the event manager, you do:
```cpp
int pfd = ev.pass_fd_to_event_manager(file_descriptor);
```
The `pfd` returned (pseudo file descriptor) can be used with the event manager from that point onwards.

From here you can submit a variety of calls using the manager (check `event_manager.hpp`).

## Teardown
You call `ev.kill()` to kill the event manager, and all submitted requests will be cancelled (so if any are unfulfilled, they will immediately show up in their respective callbacks with some error code set in `op_res_num`).

Finally the `killed_callback()` will be triggered, and you can handle any remaining tear down events for your application.