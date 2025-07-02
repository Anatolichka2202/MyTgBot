cmake_minimum_required(VERSION 3.10)
project(TelegramBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Пути к зависимостям
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(BOOST_ROOT "C:/boost/boost_1_88_0")

# Поиск библиотек
find_package(Boost 1.88.0 REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.11.2 REQUIRED)

# Настройка путей
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    "C:/boost/boost_1_88_0"
)

# Исполняемый файл
add_executable(bot
    src/bot.cpp
    src/telegram_api.cpp
)

# Линковка
target_link_libraries(bot
    PRIVATE
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)