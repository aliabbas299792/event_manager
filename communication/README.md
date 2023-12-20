These types should be used to facilitate communication between:
1. The coroutine and the event loop to process IO events (request_types.hpp)
2. The event loop and the coroutine to pass back response information (response_types.hpp)

For a given thread we'll only ever have up to one in flight request or response, so they act as one place buffers.

The return data is encapsulated in optionals rather than failing and throwing an exception, as `std::get` would, and variants are used to store the request/response data.