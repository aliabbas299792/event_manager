ROOT_PATH=$(git rev-parse --show-toplevel)
cd $ROOT_PATH
cd scripts

./compile.sh
./run_example.sh

cd $ROOT_PATH
cd build
ninja test
ninja coverage