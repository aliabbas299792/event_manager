project('event_manager', 'cpp', default_options: ['cpp_std=c++2a'])

io_uring_dep = declare_dependency(link_args: ['-luring'])
thread_dep = dependency('threads')

library('event_manager', ['src/event_manager.cpp'], dependencies: [io_uring_dep, thread_dep])