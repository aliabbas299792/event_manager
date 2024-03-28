# sanitiser_args = ['-fsanitize=address']
sanitiser_args = ['-fsanitize=undefined']
# sanitiser_args = []

general_example = executable(
  'event_loop_example',
  'event_loop_example.cpp',
  c_args: sanitiser_args,
  cpp_args: sanitiser_args,
  link_args: sanitiser_args,
  dependencies: libevent_manager_dep
)

simple_example = executable(
  'simple_coro_example',
  'simple_coro_example.cpp',
  c_args: sanitiser_args,
  cpp_args: sanitiser_args,
  link_args: sanitiser_args,
  dependencies: libevent_manager_dep
)

readme_example = executable(
  'readme_example',
  'readme_example.cpp',
  c_args: sanitiser_args,
  cpp_args: sanitiser_args,
  link_args: sanitiser_args,
  dependencies: libevent_manager_dep
)

http_example = executable(
  'http_example',
  'http_example.cpp',
  c_args: sanitiser_args,
  cpp_args: sanitiser_args,
  link_args: sanitiser_args,
  dependencies: libevent_manager_dep
)