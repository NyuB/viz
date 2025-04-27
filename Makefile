include cmake/help.mk
PRESET=gcc

# Run gtest unit tests
# Please note that this is expected to FAIL since the goal is to showcase assertions failure printing
test:
	ctest --output-on-failure --test-dir build
FORCE:

# CMake configure
configure: FORCE
	cmake -B build --preset ${PRESET} .
# CMake build
build: FORCE
	cmake --build build --target all
# Format cpp sources with clang-format
fmt:
	clang-format -i *.cpp *.hpp

# Run executable defined by main.cpp
# Not so usefull by itself but this is quickcheck that the debugee program works
run: build
	build/Main.exe