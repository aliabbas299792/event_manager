project('event_manager', 'cpp', default_options: ['cpp_std=c++2a'])

io_uring_dep = declare_dependency(link_args: ['-luring'])
thread_dep = dependency('threads')

executable('test_project', 'event_manager_tests.cpp', dependencies: [io_uring_dep, thread_dep])