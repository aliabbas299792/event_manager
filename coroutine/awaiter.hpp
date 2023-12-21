#ifndef EV_AWAITER_
#define EV_AWAITER_

#include "communication/communication_channel.hpp"
#include "task.hpp"

template <RequestType ReqT, ResponseType RespT,
          typename ReqType = ReqTypeMap<ReqT>,
          typename RespType = RespTypeMap<RespT>>
struct EvAwaiter {
  CommunicationChannel *stored_data = nullptr;
  ReqType data{};

  bool await_ready() const noexcept { return false; };
  void await_suspend(typename EvTask::Handle handle) {
    stored_data = &handle.promise().state.com_data;
    stored_data->set_req_data<ReqT>(std::move(data));
  }

  std::optional<RespType> await_resume() {
    // clearing any stored data
    data = {};
    return stored_data->get_resp_data<RespT>();
  }

  EvAwaiter(ReqType data = {}) : data(data) {}
};

#endif