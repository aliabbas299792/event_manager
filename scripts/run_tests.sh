ROOT_PATH=$(git rev-parse --show-toplevel)

cd $ROOT_PATH
cd scripts

./compile.sh

cd $ROOT_PATH
cd build
./tests/event_manager_tests