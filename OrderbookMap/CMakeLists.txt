cmake_minimum_required(VERSION 3.20)
project(Orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)


# -----------------------------
# Adding source files to a library to allow code resuability for both Main Executable and Test
# -----------------------------
add_library(orderbook_lib
    src/order_matching/Orderbook.cpp
    src/benchmarking/GenerateOrder.cpp
)
target_include_directories(orderbook_lib PUBLIC include)


# -----------------------------
# Main Executable
# ----------------------------
add_executable(Orderbook main.cpp)
target_link_libraries(Orderbook PRIVATE orderbook_lib)                              # link main executable (Orderbook) to library

# -----------------------------
# Test
# -----------------------------
enable_testing()
add_executable(OrderbookTests tests/test_orderbook.cpp)
target_link_libraries(OrderbookTests PRIVATE orderbook_lib GTest::gtest_main)       # link test executable (OrderbookTests) to library


include(GoogleTest)
gtest_discover_tests(OrderbookTests)