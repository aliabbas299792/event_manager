ROOT_PATH=$(git rev-parse --show-toplevel)

cd $ROOT_PATH
cd scripts

./compile.sh $1

cd $ROOT_PATH
cd build
./tests/event_manager_tests