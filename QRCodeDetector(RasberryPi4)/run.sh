#!/bin/bash

# Check if the 'build' directory exists
if [ -d "build" ]; then
    echo "Removing existing build directory..."
    rm -rf build
fi

# Create the 'build' directory and navigate into it
echo "Creating build directory..."
mkdir build 
cp sample.jpg build
cd build

# Run CMake and Make
echo "Running CMake and Make..."
cmake .. && make

