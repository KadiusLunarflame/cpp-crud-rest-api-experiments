cmake_minimum_required(VERSION 3.20)

project(
        rest-api
        LANGUAGES CXX
)

add_subdirectory(libs)

message(STATUS "crow include = ${CROW_INCLUDE_DIRECTORY}")
message(STATUS "libpqxx include = ${LIBPQXX_INCLUDE_DIRECTORY}")


add_subdirectory(src)