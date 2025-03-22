
#include "event_manager.hpp"

void EventManager::register_coro(EvTask&& coro) {
  coro.start();  // start it in case it hasn't been started yet

  uint64_t selected_idx = 0;
  if (_managed_coroutines_freed_idxs.size() != 0) {
    selected_idx = *_managed_coroutines_freed_idxs.begin();
    _managed_coroutines_freed_idxs.erase(selected_idx);
    _managed_coroutines[selected_idx] = std::move(coro);
  } else {
    _managed_coroutines.push_back(std::move(coro));
    selected_idx = _managed_coroutines.size() - 1;
  }

  // we are storing the index in the vector as metadata
  _managed_coroutines[selected_idx].set_coro_metadata(selected_idx);
}
