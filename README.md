# Event Manager
Simple liburing based library which uses callbacks for I/O events.

As it stands, memory leaks are possible if there are `read()` requests submitted when you `kill()` the server, since they won't be freed in that case.

## Todo:
- Other advantage to this approach is that all requests currently submitted can be cancelled in Kernel 5.19+
  - Using `pfd_freed_pfds` to avoid freed `pfd`s and `pfd_to_data`, you can loop through all active `pfd`s
  - For each `pfd`, you can use a liburing function using `IORING_ASYNC_CANCEL_FD` (described [here](https://git.kernel.dk/cgit/linux-block/commit/?h=for-5.19/io_uring&id=4bf94615b8886305199ed5755cb72fea88258d15)) to cancel using the `fd`
- This means that any event manager instance can be cleanly destroyed without memory leaks
- `IORING_ASYNC_CANCEL_ANY` seemed to help with this, but we need to know when we're done
  - Use `IORING_ASYNC_CANCEL_FD` and keep a counter for each `fd`, increment/decrement the counter for every active request, so we'll know when we're done