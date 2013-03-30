project(yandex_intern_2013)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

cmake_minimum_required(VERSION 2.8)

include(YandexContestCommon)

bunsan_use_bunsan(common)
bunsan_use_boost(system filesystem serialization unit_test_framework program_options thread)
bunsan_use(yandex_contest_common yandex_contest_system)

bunsan_add_shared_library(${PROJECT_NAME}
    src/lib/generate.cpp
    src/lib/isSorted.cpp
    src/lib/Sorter.cpp
    src/lib/sorters/BlockSorter.cpp
    src/lib/sorters/InMemorySorter.cpp
    src/lib/sorters/MergeSorter.cpp

    src/lib/detail/radixSort.cpp
    src/lib/detail/stdSort.cpp
    src/lib/detail/MemoryMap.cpp
    src/lib/detail/FileMemoryMap.cpp
    src/lib/detail/copyFile.cpp
    src/lib/detail/io.cpp
)

bunsan_use(${PROJECT_NAME})

bunsan_add_executable(is_sorted
    src/bin/is_sorted.cpp
)

bunsan_add_executable(binary2text
    src/bin/binary2text.cpp
)

bunsan_add_executable(text2binary
    src/bin/text2binary.cpp
)

bunsan_add_executable(generate
    src/bin/generate.cpp
)

bunsan_add_executable(sort src/bin/sort.cpp)

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME})

bunsan_include_tests()
