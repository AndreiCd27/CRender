#!/bin/bash

# See Linux OpenGL & GLFW dependencies
# https://medium.com/geekculture/a-beginners-guide-to-setup-opengl-in-linux-debian-2bfe02ccd1e

# For Debian based distributions

# OpenGL
sudo apt-get install cmake pkg-config
sudo apt-get install mesa-utils libglu1-mesa-dev freeglut3-dev mesa-common-dev
sudo apt-get install libglew-dev libglfw3-dev libglm-dev
sudo apt-get install libao-dev libmpg123-dev

#wayland-scanner
sudo apt install libwayland-bin libwayland-dev
sudo apt install libxkbcommon-dev

#install xinerama for cmake
sudo apt install libxinerama-dev (if not installed already)
#install xcursor for cmake
sudo apt install libxcursor-dev (if not installed already)

# GLFW
cd /usr/local/lib/
sudo git clone https://github.com/glfw/glfw.git
cd glfw
sudo cmake .
sudo make
sudo make install