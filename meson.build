project('event_manager', 'cpp', default_options: ['cpp_std=c++20'])

io_uring_dep = declare_dependency(link_args: ['-luring'])
thread_dep = dependency('threads')

source_files = ['event_loop/core.cpp', 'event_loop/io_ops.cpp']

root_inc = include_directories('.')

libevent_manager = library(
  'event_manager',
  source_files,
  dependencies: [io_uring_dep, thread_dep]
)

libevent_manager_dep = declare_dependency(
  include_directories: root_inc,
  link_with: libevent_manager
)

subdir('examples')
subdir('tests')
