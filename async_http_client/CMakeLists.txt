# CMakeList.txt : CMake project for async_http_client, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)
set(Boost_INCLUDE_DIR path-to-boost)
set(Boost_LIBRARY_DIR path-to-boost + /stage/lib")
find_package(Boost)ss
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})


add_executable(async_http_client "main.cpp" "http/http.cpp" )
target_link_libraries(async_http_client ${Boost_LIBRARIES} )




# TODO: Add tests and install targets if needed.
