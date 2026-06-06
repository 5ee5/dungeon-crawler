cmake_minimum_required(VERSION 3.16)
project(DungeonCrawler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)

add_executable(dungeon_crawler
    src/main.cpp
    src/Game.cpp
    src/Dungeon.cpp
    src/Entities.cpp
    src/Renderer.cpp
)

target_include_directories(dungeon_crawler PRIVATE src)
target_link_libraries(dungeon_crawler PRIVATE ${CURSES_LIBRARIES})

