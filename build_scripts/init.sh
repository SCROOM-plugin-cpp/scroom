# First get the scroom repository

# Fetch all required packages on clone too
apt update
apt upgrade
apt install -y git gawk libboost-test-dev libboost-dev libboost-filesystem-dev libboost-program-options-dev libboost-system-dev libboost-thread-dev libgtest-dev google-mock libcairo2-dev libglade2-dev libglib2.0-dev libgtk2.0-dev libtiff5-dev pkg-config doxygen graphviz ccache libc6-dev

cd scroom
autoreconf -i
cd ..

# Then configure it
path=$(pwd)/scroom-install

mkdir scroom-install
cd scroom
mkdir scroom-build
cd scroom-build
../configure CC="ccache gcc" CXX="ccache g++" --prefix $path
cd ../..

# Next, test and build scroom
cd ./scroom/scroom-build
make -k -j1 check
make -k -j1 install
cd ../..

# Then maybe add a run shortcut
if [ ! -f "run-scroom.sh" ]; then
  echo "./scroom-install/bin/scroom" > run-scroom.sh
  chmod 755 run-scroom.sh
else
  echo "Skipping run script..."
fi
