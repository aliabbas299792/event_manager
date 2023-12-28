project('event_manager', 'cpp', default_options: ['cpp_std=c++20'])

io_uring_dep = declare_dependency(link_args: ['-luring'])
thread_dep = dependency('threads')

general_source_files = ['event_loop/core.cpp', 'event_loop/io_ops.cpp']

sanitiser_args = ['-fsanitize=address']
# sanitiser_args = ['-fsanitize=undefined']
# sanitiser_args = []

# libevent_manager = executable('event_manager', source_files, dependencies: [io_uring_dep, thread_dep])
example_event_manager = executable(
  'example_event_manager',
  ['main.cpp'] + general_source_files,
  c_args: sanitiser_args,
  cpp_args: sanitiser_args,
  link_args: sanitiser_args,
  dependencies: [io_uring_dep, thread_dep]
)
# libevent_manager_dep = declare_dependency(link_with: libevent_manager)

# subdir('tests')

communication_tests = executable(
  'communication_tests',
  'communication/communication_tests.cpp',
  dependencies: [io_uring_dep, thread_dep]
)
test('Communication Tests', communication_tests)

event_loop_tests = executable(
  'event_loop_tests',
  ['event_loop/event_loop_tests.cpp'] + general_source_files,
  dependencies: [io_uring_dep, thread_dep]
)
test('Event Loop Tests', event_loop_tests)

event_loop_example = executable(
  'event_loop_example',
  ['event_loop/event_loop_example.cpp'] + general_source_files,
  dependencies: [io_uring_dep, thread_dep]
)
test('Event Loop Example', event_loop_example)

# subdir('example')