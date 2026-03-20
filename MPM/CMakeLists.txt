# 1. 将 MPM 文件夹下的源文件打包成静态库，命名为 MPM_Module
add_library(MPM_Module STATIC
    src/WavDecoder.cpp
    src/RingBuffer.cpp
    src/MediaProgressManager.cpp
)

# 2. 告诉系统，这个模块的头文件在 MPM/include 里面
# PUBLIC 意味着主程序连上这个库之后，也能直接引用这些头文件
target_include_directories(MPM_Module PUBLIC include)
# --- 专门用来独立测试的程序 ---
# 把 test_mpm.cpp 编译成一个叫 TestMPM 的独立程序
add_executable(TestMPM test_mpm.cpp)

# 让你的测试程序能使用你的 MPM_Module 零件库
target_link_libraries(TestMPM PRIVATE MPM_Module)
