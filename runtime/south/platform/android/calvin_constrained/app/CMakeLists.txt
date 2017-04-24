cmake_minimum_required(VERSION 3.4.1)

cmake_policy(SET CMP0003 OLD)
cmake_policy(SET CMP0026 OLD)

add_library(
             calvin_constrained

             SHARED

             ../../../../../../main.c
             ../../../../../../runtime/south/platform/android/cc_platform_android.c
             ../../../../../../runtime/south/platform/android/cc_jni_api.c
             ../../../../../../runtime/south/platform/android/calvinsys/accelerometer.c
             ../../../../../../main.c
             ../../../../../../calvinsys/cc_calvinsys.c
             ../../../../../../cc_api.c
             ../../../../../../runtime/north/cc_common.c
             ../../../../../../runtime/north/scheduler/np_scheduler/cc_scheduler.c
             ../../../../../../runtime/north/cc_node.c
             ../../../../../../runtime/north/cc_proto.c
             ../../../../../../runtime/north/cc_transport.c
             ../../../../../../runtime/south/transport/fcm/cc_transport_fcm.c
             ../../../../../../runtime/south/transport/socket/cc_transport_socket.c
             ../../../../../../runtime/north/cc_tunnel.c
             ../../../../../../runtime/north/cc_link.c
             ../../../../../../runtime/north/cc_actor.c
             ../../../../../../runtime/north/cc_port.c
             ../../../../../../runtime/north/cc_fifo.c
             ../../../../../../runtime/north/cc_token.c
             ../../../../../../runtime/north/cc_msgpack_helper.c
             ../../../../../../msgpuck/msgpuck.c
             ../../../../../../actors/cc_actor_identity.c
             ../../../../../../actors/cc_actor_button.c
             ../../../../../../actors/cc_actor_gpioreader.c
             ../../../../../../actors/cc_actor_gpiowriter.c
             ../../../../../../actors/cc_actor_temperature.c
)

find_library(
            log-lib
            log
)

target_link_libraries(
             calvin_constrained
             micropython
             ${log-lib}
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -landroid")