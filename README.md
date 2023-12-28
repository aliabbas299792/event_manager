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

# Todo
0. Add documentation for how to add more operations
1. Add in these operations:
  - `openat`
  - `opendir`
  - `statx`
  - `unlink`
  - `rename`