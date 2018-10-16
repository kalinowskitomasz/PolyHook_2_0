cd capstone
if [ "$(uname)" == "Darwin" ]; then # Don't build 32 bit version on Mac as it is deprecated
  mkdir build32
  cd build32
  cmake -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_BUILD_TYPE=Release ../
  make
  cmake -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_DEBUG_POSTFIX=_debug ../
  make
  cd ../
fi
mkdir build64
cd build64
cmake -DCMAKE_BUILD_TYPE=Release ../
make
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_DEBUG_POSTFIX=_debug ../
make
