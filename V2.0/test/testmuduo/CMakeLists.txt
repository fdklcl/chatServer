#g++ -o server -g *.cpp -l/usr/include -L/usr/lib -muduo_net -lmuduo_base -lpthread
#bin   可执行文件
#lib   中间库文件
#include  头文件
#src    源文件
#build   编译过程中产生的临时文件
#example  测试代码
#thirdparty  第三方库
#CMakeLists.txt
#autobuild.sh  一键编译


# 配置头文件的搜索路径
#include_directories()

#配置库文件的搜索路径
#link_directories()

#设置需要编译的源文件列表
set(SRC_LIST testmuduo.cpp)
#设置可执行文件最终存储的路径  PROJECT_SOURCE_DIR
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#把.指定路径下的所有源文件名字放入变量SRC_LIST里面
#aux_source_directory(. SRC_LIST)
#add_executable(server testmuduo.cpp)
add_executable(server ${SRC_LIST})

#配置需要链接的库
target_link_libraries(server muduo_net muduo_base pthread)