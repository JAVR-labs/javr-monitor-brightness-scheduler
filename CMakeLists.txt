cmake_minimum_required(VERSION 3.16)
project(brightness_scheduler)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(nlohmann_json)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(brightness_scheduler main.cpp)
target_link_libraries(brightness_scheduler PRIVATE
        Qt6::Core
        nlohmann_json::nlohmann_json
)