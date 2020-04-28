# First get the scroom repository
if [ "1" == "1" ]; then
  # Fetch all required packages on clone too
  apt update
  apt upgrade
  apt install git gawk libboost-test-dev libboost-dev libboost-filesystem-dev libboost-program-options-dev libboost-system-dev libboost-thread-dev libgtest-dev google-mock libcairo2-dev libglade2-dev libglib2.0-dev libgtk2.0-dev libtiff5-dev pkg-config doxygen graphviz ccache libc6-dev
  apt < yes

  # git clone --depth 200 https://github.com/kees-jan/scroom.git
  cd scroom
  autoreconf -i
  cd ..
else
  echo "Skipping scroom setup..."
fi

# Then maybe configure it
path=$(pwd)/scroom-install
if [ "1" == "1" ]; then
  mkdir scroom-install
  cd scroom
  mkdir scroom-build
  cd scroom-build
  ../configure CC="ccache gcc" CXX="ccache g++" --prefix $path
  cd ../..
  build=1
else
  echo "Skipping scroom build configuration..."
  if [ "$1" == "true" ]; then
    build=1
  else
    build=0
  fi
fi

# Then maybe build it
if [ $build -eq 1 ]; then
  cd ./scroom/scroom-build
  make -k -j install
  cd ../..
else
  echo "Skipping scroom build..."
fi

# Then maybe add a run shortcut
if [ ! -f "run-scroom.sh" ]; then
  echo "./scroom-install/bin/scroom" > run-scroom.sh
  chmod 755 run-scroom.sh
else
  echo "Skipping run script..."
fi

# Then get our plugin
if [ "1" == "2" ]; then
  # HTTPS
  git clone https://github.com/SCROOM-plugin-cpp/SCROOM-plugin-cpp.git
  # SSH:
  # git clone git@github.com:SCROOM-plugin-cpp/SCROOM-plugin-cpp.git
  cd SCROOM-plugin-cpp
  autoreconf -i
  cd ..
else
  echo "Skipping plugin setup..."
fi

# Then maybe configure our plugin
if [ "1" == "2" ]; then
  cd ./SCROOM-plugin-cpp
  mkdir plugin-build
  cd plugin-build
  ../configure CC="ccache gcc" CXX="ccache g++" CPPFLAGS=-I$path/include/ LDFLAGS="-L$path/lib -Wl,-z,defs"
  cd ../..
else
  echo "Skipping plugin build configuration..."
fi

# Then maybe make a plugin build script
if [ "1" == "2" ]; then
  echo "cd ./SCROOM-plugin-cpp/plugin-build" >> build-plugin.sh
  echo "make -k -j" >> build-plugin.sh
  echo "cd ../.." >> build-plugin.sh
  echo "rm -f ./scroom-install/lib/scroom/libspexample.*" >> build-plugin.sh
  echo "cp ./SCROOM-plugin-cpp/plugin-build/plugins/example/src/.libs/* ./scroom-install/lib/scroom/" >> build-plugin.sh
  echo "echo \"Done! Run scroom using the generated 'run-scroom.sh' script.\"" >> build-plugin.sh
  chmod 755 build-plugin.sh
else
  echo "Skipping plugin build script..."
fi

echo "Build the plugin by running the generated 'build-plugin.sh' script."
