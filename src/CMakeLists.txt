cmake_minimum_required(VERSION 3.24)
project(cpp_crud_rest_api_experiments)

set(CMAKE_CXX_STANDARD 20)

add_executable(cpp_crud_rest_api_experiments main.cpp)


target_link_libraries(cpp_crud_rest_api_experiments
        PRIVATE ${THREADS_LINK_TARGET}
        PRIVATE pqxx)
target_include_directories(cpp_crud_rest_api_experiments SYSTEM
        PRIVATE ${CROW_INCLUDE_DIRECTORY}
        PRIVATE ${BOOST_INCLUDE_DIRECTORY}
        PRIVATE ${LIBPQXX_INCLUDE_DIRECTORY})
