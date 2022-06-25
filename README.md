# Event Manager
Simple liburing based library which uses callbacks for I/O events.

As it stands, memory leaks are possible if there are `read()` requests submitted when you `kill()` the server, since they won't be freed in that case.

## Todo:
- Potentially add some flexible method of making sockets, so that a `pfd` is returned
- Since the event manager only uses `pfd`s, make a method to return the `fd` corresponding to that `pfd` for further processing
    - i.e, like for `setsockopt` and `bind`, no need to add individual methods for those
- Other advantage to this approach is that all requests currently submitted can be cancelled in Kernel 5.19+
  - Using `pfd_freed_pfds` to avoid freed `pfd`s and `pfd_to_data`, you can loop through all active `pfd`s
  - For each `pfd`, you can use a liburing function using `IORING_ASYNC_CANCEL_FD` (described (https://git.kernel.dk/cgit/linux-block/commit/?h=for-5.19/io_uring&id=4bf94615b8886305199ed5755cb72fea88258d15)[here]) to cancel using the `fd`
- This means that any event manager instance can be cleanly destroyed without memory leaks