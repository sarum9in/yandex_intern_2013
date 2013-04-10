project(yandex_intern_2013)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

cmake_minimum_required(VERSION 2.8)

include(YandexContestCommon)

bunsan_use_bunsan(common)
bunsan_use_boost(system filesystem serialization unit_test_framework thread)
bunsan_use(yandex_contest_common yandex_contest_system)

bunsan_add_shared_library(${PROJECT_NAME}
    src/lib/generate.cpp
    src/lib/isSorted.cpp
    src/lib/Sorter.cpp
    src/lib/sorters/BalancedSplitSorter.cpp
    src/lib/sorters/InMemorySorter.cpp
    src/lib/sorters/SplitMergeSorter.cpp

    src/lib/detail/bit.cpp
    src/lib/detail/radixSort.cpp
    src/lib/detail/stdSort.cpp
    src/lib/detail/MemoryMap.cpp
    src/lib/detail/FileMemoryMap.cpp
    src/lib/detail/copyFile.cpp
    src/lib/detail/io.cpp
    src/lib/detail/SequencedInputBuffer.cpp
    src/lib/detail/SequencedOutputBuffer.cpp
    src/lib/detail/Timer.cpp
)

bunsan_use(${PROJECT_NAME})

yandex_contest_add_binary_targets(bin_targets
    binary2text
    generate_biased
    generate_unbiased
    is_sorted
    sort
    text2binary

    balanced_split_sort
    in_memory_sort
    split_merge_sort
)

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME} ${bin_targets})

bunsan_include_tests()
