# 定义一个变量，包含该目录下的所有文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

#生成可执行文件，连接库
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
# 表示 CharServer 这个目标程序需要链接 muduo_net、muduo_base 和 pthread 三个库文件  
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)