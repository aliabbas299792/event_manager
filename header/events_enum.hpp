#ifndef EVENTS_ENUM
#define EVENTS_ENUM

enum class events {
  WRITE, READ, ACCEPT, SHUTDOWN, CLOSE, EVENT,
  KILL = 999
};

#endif