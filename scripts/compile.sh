ROOT_PATH=$(git rev-parse --show-toplevel)
cd $ROOT_PATH

EXTRA_OPTS=

if [ "$1" == "debug" ]; then
  EXTRA_OPTS=-Db_sanitize=address
fi

meson build $EXTRA_OPTS -Db_coverage=true
cd build
ninja