#!/bin/bash

cmake -B build -S .
cmake --build build
sudo cmake --install build