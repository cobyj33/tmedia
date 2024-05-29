if (( $EUID != 0 )); then
    SUDO='sudo'
fi

$SUDO apt-get update
$SUDO apt-get install git build-essential cmake pkg-config libavdevice-dev libncurses-dev

git submodule update --init || exit 1

mkdir -p build || exit 1
cd build || exit 1
cmake ../
make -j$(nproc)
