# 定义测试程序
set(TEST_PROGRAMS
    hello
    mprotect
    fork
    timetest
    test_main    # 包含 test_main
    test_sandbox
)

# 创建所有测试程序，并特别处理 test_main
foreach(program ${TEST_PROGRAMS})
    if(${program} STREQUAL "test_main")
        add_executable(${program} src/test.c)    # test_main 使用 test.c
    else()
        add_executable(${program} src/${program}.c)
    endif()
endforeach()

# 为特定程序添加额外的链接库（需要显式链接 libvmpl 的依赖库）
target_link_libraries(mprotect dune vmpl dict hotcalls)
target_link_libraries(timetest dune vmpl dict hotcalls)

# 其他程序链接通用库
foreach(program ${TEST_PROGRAMS})
    if(NOT ${program} STREQUAL "mprotect" AND NOT ${program} STREQUAL "timetest")
        target_link_libraries(${program} dune vmpl dict hotcalls pthread)
    endif()
endforeach()