project('event_manager', 'cpp', default_options: ['cpp_std=c++2a'])

io_uring_dep = declare_dependency(link_args: ['-luring'])
thread_dep = dependency('threads')

source_files = ['src/core.cpp', 'src/event_loop.cpp', 'src/pfd_methods.cpp', 'src/queue_ops.cpp', 'src/submit_ops.cpp', 'src/awaitables.cpp']

libevent_manager = library('event_manager', source_files, dependencies: [io_uring_dep, thread_dep])
libevent_manager_dep = declare_dependency(link_with: libevent_manager)

subdir('tests')
subdir('example')