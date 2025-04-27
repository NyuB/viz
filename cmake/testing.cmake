include(CTest)

# Silence a warning about missing DartConfiguration.tcl file when running CTest
# from vscode (see
# https://github.com/microsoft/vscode-cmake-tools/issues/3917#issuecomment-2381136228)
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/cd430b47a54841ec45d64d2377d7cabaf0eba610.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

function(test_with_gtest TestTarget)
  target_link_libraries(${TestTarget} PRIVATE GTest::gtest_main)
  target_link_libraries(${TestTarget} PRIVATE GTest::gmock)
  gtest_discover_tests(${TestTarget}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()
