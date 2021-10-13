cmake_minimum_required(VERSION 3.10)

#Project Name
project(File_Transfer VERSION 0.6
					  DESCRIPTION "CLI program for transferring files."
					  LANGUAGES CXX)

cmake_policy(SET CMP0079 NEW)

#Directories
add_subdirectory (src/connectors)
add_subdirectory (src/filemanager)
add_subdirectory (src/main)
include_directories (src/main src/filemanager src/connectors)

#Libraries
#target_link_libraries (NFT PUBLIC FM Client Server)

#Executables
add_executable (filetransferer ./src/main/serv.cpp)
target_link_libraries (filetransferer PUBLIC NFT)
target_compile_features (filetransferer PUBLIC cxx_std_17)

