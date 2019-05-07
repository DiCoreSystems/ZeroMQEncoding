cmake_minimum_required(VERSION 3.10.2)
project(capnzero_eval)

## Use c++ 11x std
add_definitions(-std=c++11)

find_package(Threads)
find_package(catkin REQUIRED COMPONENTS capnzero)
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES capnzero-eval-msgs
  CATKIN_DEPENDS capnzero
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

############# Evaluation Software / Msg Sender

add_executable(${PROJECT_NAME}
  src/CapnZeroEvaluation.cpp 
  src/Statistics.cpp
)

target_link_libraries(${PROJECT_NAME}
  capnzero-eval-msgs
  ${catkin_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

############# Evaluation Software / Msg Echo

add_executable(Parrot
  src/Parrot.cpp
)

target_link_libraries(Parrot
  capnzero-eval-msgs
  ${catkin_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

############# Evaluation Message
find_package(CapnProto REQUIRED)
set(capnzero_eval_message_folder "${CMAKE_CURRENT_SOURCE_DIR}/msg")
file(GLOB_RECURSE capnzero_eval_msgs ${capnzero_eval_message_folder} *.capnp)
set(CAPNPC_SRC_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/msg)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/capnzero-eval-msgs")
set(CAPNPC_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/capnzero-eval-msgs")
capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS ${capnzero_eval_msgs})

add_library(capnzero-eval-msgs
  ${CAPNP_SRCS}
)