# 最低 CMake 版本要求  
cmake_minimum_required(VERSION 3.10) 
# 项目名称 
project(MyProject)

#配置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

#配置头文件搜索路径
# include directories()
#配置库文件搜索路径
# link directories()

# 设置需要编译的源文件列表  
# 这里定义了 SRC_LIST 变量，包含了 muduo_server.cpp  
set(SRC_LIST muduo_server.cc)  

# 把当前路径下的所有源文件名字放入变量名 SRC_LIST 里面  
# aux_source_directory(. SRC_LIST)  

# 设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 添加可执行文件，server 是要生成的目标程序，使用 SRC_LIST 中的文件  
add_executable(server ${SRC_LIST})  

# 表示 server 这个目标程序需要链接 muduo_net、muduo_base 和 pthread 三个库文件  
target_link_libraries(server muduo_net muduo_base pthread)  