# EvTask
- You can use it for coroutines, and await on awaitable objects in a function with this return type
- You can await on it, and it will return an integer
- If you await on it, then the current coroutine will be suspended if the inner coroutine is suspended
  - This leads to a semblance of sequential execution for a stack of coroutine calls

# Todo
1. Jot down the outline of the entire multi stage tear down sequence for the event loop, and have a look at how it was done for the other event_manager, since it seemed to work there
2. Implement the above
3. Add more error handling, since currently, especially the optional stuff, isn't used very much (this might need to be removed too)
4. Add a system to the event loop that checks if a request has been somehow looped through a bunch of times without being seen, this may have been a problem in a previous iteration of this system
5. Think a way of relatively efficiently queueing up multiple operations, and submitting them all and waiting on them all simultaneously - something like the `submit_and_wait()` function thought of earlier which took a lambda would be good
  - And on this topic, you'll need to document the user side visitor pattern type thing users would realistically need to use this system
6. Add in these operations:
  - `openat`
  - `opendir`
  - `statx`
  - `unlink`
  - `rename`
