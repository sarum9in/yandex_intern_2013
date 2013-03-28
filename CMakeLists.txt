project(yandex_intern_2013)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

cmake_minimum_required(VERSION 2.8)

include(BunsanCommon)

bunsan_use_bunsan(common)
bunsan_use_boost(system filesystem serialization unit_test_framework program_options thread)

bunsan_add_executable(is_sorted
    src/bin/is_sorted.cpp
)

bunsan_add_executable(binary2text
    src/bin/binary2text.cpp
)

bunsan_add_executable(text2binary
    src/bin/text2binary.cpp
)

#bunsan_install_headers()
#bunsan_install_targets(${PROJECT_NAME})

bunsan_include_tests()
