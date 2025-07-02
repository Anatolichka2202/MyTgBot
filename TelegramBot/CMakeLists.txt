cmake_minimum_required(VERSION 3.15)
project(TelegramBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(TgBot REQUIRED)
target_link_libraries(bot PRIVATE TgBot::TgBot)

add_executable(bot
    src/bot.cpp
    src/telegram_api.cpp
)

target_include_directories(bot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bot PRIVATE
    Boost::boost
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)