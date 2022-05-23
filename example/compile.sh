# this assumes the library was already compiled via ./compile.sh in the parent directory
clang++-14 example.cpp -o example -L../build -levent_manager -rpath ../build