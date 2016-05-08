#----------------------------------------------------------------------
#
#  Single example osmium-contrib CMakeLists.txt
#  https://github.com/osmcode/libosmium/wiki/Using-Libosmium-in-your-own-projects
#
#----------------------------------------------------------------------
project(osmium-history-parser)

cmake_minimum_required(VERSION 2.8.5)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

include(FindOsmium)
#find_package(Osmium REQUIRED)
find_package(Osmium REQUIRED COMPONENTS pbf xml io)
#if (NOT OSMIUM_FOUND)
#    message(FATAL_ERROR "Osmium library not found!")
#else()
#    message("Found Osmium library in " ${OSMIUM_INCLUDE_DIRS})
#endif()
message("Found Osmium library in " ${OSMIUM_INCLUDE_DIR})
include_directories(${OSMIUM_INCLUDE_DIRS})

include(common)

#----------------------------------------------------------------------

add_executable(user-edit-history user_edit_history.cpp)
target_link_libraries(user-edit-history ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})

add_executable(user-tag-edit-history user_tag_edit_history.cpp)
target_link_libraries(user-tag-edit-history ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})

add_executable(user-uid-name-map user_uid_name_map.cpp)
target_link_libraries(user-uid-name-map ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})

#----------------------------------------------------------------------

