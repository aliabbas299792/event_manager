# sanitiser_args = ['-fsanitize=address']
sanitiser_args = ['-fsanitize=undefined']
other_args = ['--coverage']
# sanitiser_args = []

example_sources = {
  'event_loop_example': 'event_loop_example.cpp',
  'simple_coro_example': 'simple_coro_example.cpp',
  'readme_example': 'readme_example.cpp',
  'http_example': 'http_example.cpp',
}

foreach name, source : example_sources
  executable(
    name,
    source,
    c_args: sanitiser_args,
    cpp_args: sanitiser_args + other_args,
    link_args: sanitiser_args + other_args,
    dependencies: libevent_manager_dep,
  )
endforeach
