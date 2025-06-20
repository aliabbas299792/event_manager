sanitiser_args = ['-fsanitize=address']
other_args = ['--coverage']

communication_tests = executable(
  'communication_tests',
  'communication_tests.cpp',
  c_args: sanitiser_args,
  cpp_args: sanitiser_args + other_args,
  link_args: sanitiser_args + other_args,
  dependencies: libevent_manager_dep
)

event_loop_tests = executable(
  'event_loop_tests',
  'event_loop_tests.cpp',
  c_args: sanitiser_args,
  cpp_args: sanitiser_args + other_args,
  link_args: sanitiser_args + other_args,
  dependencies: libevent_manager_dep
)

na_tests = executable(
  'na_tests',
  'na_tests.cpp',
  c_args: sanitiser_args,
  cpp_args: sanitiser_args + other_args,
  link_args: sanitiser_args + other_args,
  dependencies: libevent_manager_dep
)

test('Communication Tests', communication_tests)
test('Event Loop Tests', event_loop_tests)
test('NA Tests', na_tests)
