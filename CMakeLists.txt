cmake_minimum_required(VERSION 3.10)
project(repo_uploader VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(repo_uploader src/main.cpp)

if(MSVC)
  target_compile_options(repo_uploader PRIVATE /W4 /permissive-)
else()
  target_compile_options(repo_uploader PRIVATE -Wall -Wextra -pedantic)
endif()
