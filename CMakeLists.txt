cmake_minimum_required(VERSION 3.15)
project(Zos)

set(CMAKE_CXX_STANDARD 14)

add_executable(Zos main.cpp FileSystem.cpp FileSystem.h DirectoryItem.h SuperBlock.h PseudoInode.h Utils.cpp Utils.h SuperBlock.cpp PseudoInode.cpp DirectoryItem.cpp Bitmap.cpp Bitmap.h Indirect.cpp Indirect.h)