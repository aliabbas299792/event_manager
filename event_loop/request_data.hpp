#ifndef REQUEST_DATA_
#define REQUEST_DATA_

#include <bits/types/struct_iovec.h>
#include <cstdint>
#include <sys/socket.h>

#include "coroutine/task.hpp"
#include "event_loop/parameter_packs.hpp"

struct RequestData {
  EvTask::Handle handle{};
  uint64_t coro_idx{}; // index in the managed coroutines vector in the event manager
  bool *coro_finished{}; // pointer to a field in a task_status object managed as a unique ptr
  RequestType req_type{};

  union {
    ReadParameterPack read_data;
    WriteParameterPack write_data;
    CloseParameterPack close_data;
    ShutdownParameterPack shutdown_data;
    ReadvParameterPack readv_data;
    WritevParameterPack writev_data;
    AcceptParameterPack accept_data;
    ConnectParameterPack connect_data;
    OpenatParameterPack openat_data;
    StatxParameterPack statx_data;
    UnlinkatParameterPack unlinkat_data;
    RenameatParameterPack renameat_data;
  } specific_data{};
};

#endif