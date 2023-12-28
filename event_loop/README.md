# How to use submit and wait
You need to provide a handler with this signature: `(RequestType, CommunicationChannel*) -> void`.

In the handler you'll want to use a visitor pattern like the one below to access the response data:
```cpp
auto queue = ev->make_request_queue();

// do stuff like queue->queue_read(...)

co_await ev->submit_and_wait(
    queue, [](RequestType req_type, CommunicationChannel *channel) {
      switch (req_type) {
      case RequestType::READ: {
        break;
      }
      case RequestType::WRITE: {
        auto data = channel->consume_resp_data<RequestType::WRITE>();
        std::cout << "wrote         "  << data->bytes_wrote << "\n"
                  << "bytes for fd " << data->fd << "\n";
        break;
      }
      case RequestType::CLOSE: {
        break;
      }
      case RequestType::SHUTDOWN: {
        break;
      }
      case RequestType::READV: {
        break;
      }
      case RequestType::WRITEV: {
        break;
      }
      case RequestType::ACCEPT: {
        break;
      }
      case RequestType::CONNECT: {
        break;
      }
      }
    });
```
Where `ev` is `EventManager*`, a pointer to the event manager.