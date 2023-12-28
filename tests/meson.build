communication_tests = executable(
  'communication_tests',
  'communication_tests.cpp',
  dependencies: libevent_manager_dep
)

event_loop_tests = executable(
  'event_loop_tests',
  'event_loop_tests.cpp',
  dependencies: libevent_manager_dep
)

test('Communication Tests', communication_tests)
test('Event Loop Tests', event_loop_tests)