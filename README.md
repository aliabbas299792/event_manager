# Event Manager
Simple liburing based library which uses callbacks for I/O events.

As it stands, memory leaks are possible if there are `read()` requests submitted when you `kill()` the server, since they won't be freed in that case.