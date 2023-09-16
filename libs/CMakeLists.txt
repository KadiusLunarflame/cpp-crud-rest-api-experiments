#--------------------------------------------------
# threads


find_package(Threads)
set(THREADS_LINK_TARGET ${CMAKE_THREAD_LIBS_INIT} PARENT_SCOPE)


#--------------------------------------------------
# crow

#find_package(ZLIB) #http compression?

find_package(
        Boost 1.70
        COMPONENTS system thread
        REQUIRED)
set(BOOST_INCLUDE_DIRECTORY
        ${Boost_INCLUDE_DIR}
        PARENT_SCOPE)

set(CROW_BUILD_EXAMPLES OFF)
set(CROW_BUILD_TESTS OFF)

add_subdirectory(crow)

set(CROW_INCLUDE_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/crow/include"
        PARENT_SCOPE)
#--------------------------------------------------
#libpqxx

#set(BUILD_DOC OFF)
set(SKIP_BUILD_TEST ON)
add_subdirectory(libpqxx)

set(LIBPQXX_INCLUDE_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/libpqxx/include"
        PARENT_SCOPE)
#--------------------------------------------------