ROOT_PATH=$(git rev-parse --show-toplevel)

cd $ROOT_PATH
cd scripts

./compile.sh

cd $ROOT_PATH
cd build
./example/event_manager_example