cmake_minimum_required(VERSION 3.0.2)
project(rosGoalClient)
find_package(catkin REQUIRED COMPONENTS
  actionlib
  move_base_msgs
  roscpp
  std_srvs
  message_generation
  std_msgs
)
find_library(MYSQL_LIB mysqlclient /usr/lib/x86_64-linux-gnu/)

find_package(Threads REQUIRED)

add_service_files(FILES bot3gpio.srv)
generate_messages(DEPENDENCIES std_msgs)
		
# add_message_files(
#   FILES
#   Message1.msg
#   Message2.msg
# )

# add_service_files(
#   FILES
#   Service1.srv
#   Service2.srv
# )

# add_action_files(
#   FILES
#   Action1.action
#   Action2.action
# )

# generate_messages(
#   DEPENDENCIES
#   move_base_msgs
# )

# generate_dynamic_reconfigure_options(
#   cfg/DynReconf1.cfg
#   cfg/DynReconf2.cfg
# )

catkin_package(
  INCLUDE_DIRS include
  LIBRARIES rosGoalClient
  CATKIN_DEPENDS actionlib move_base_msgs roscpp std_srvs
  DEPENDS system_lib
)

include_directories(
 include
  ${catkin_INCLUDE_DIRS}
 /usr/include/mysql
)

add_executable(rosGoalClient src/rosGoalClient.cpp)

if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(rosGoalClient ${CMAKE_THREAD_LIBS_INIT})
endif()

target_link_libraries(rosGoalClient
  Threads::Threads
  ${catkin_LIBRARIES}
  ${MYSQL_LIB}
)

add_dependencies(rosGoalClient ${rosGoalClient_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

