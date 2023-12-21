project('event_manager', 'cpp', default_options: ['cpp_std=c++20'])

io_uring_dep = declare_dependency(link_args: ['-luring'])
thread_dep = dependency('threads')

source_files = ['main.cpp']

# libevent_manager = executable('event_manager', source_files, dependencies: [io_uring_dep, thread_dep])
example_event_manager = executable('example_event_manager', source_files, dependencies: [io_uring_dep, thread_dep])
# libevent_manager_dep = declare_dependency(link_with: libevent_manager)

# subdir('tests')

communication_tests = executable('communication_tests', 'communication/communication_tests.cpp', dependencies: [io_uring_dep, thread_dep])
test('Communication Tests', communication_tests)

# subdir('example')