project(audioapp)

cmake_minimum_required(VERSION 2.4.6..3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/comm_audio/comm_audio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comm_audio/comm_audio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE

    ${HI3519_ROOT}/lib/libsecurec.so
    ${HI3519_ROOT}/lib/libmp3_enc.so
    ${HI3519_ROOT}/lib/libmp3_lame.so
    ${HI3519_ROOT}/lib/libmp3_dec.so
    ${HI3519_ROOT}/lib/libvoice_engine.so
    ${HI3519_ROOT}/lib/libupvqe.so
    ${HI3519_ROOT}/lib/libdnvqe.so

    # mpi
    ${HI3519_ROOT}/lib/libhi_mpi_audio.so
    ${HI3519_ROOT}/lib/libhi_mpi_audio_adp.so
    ${HI3519_ROOT}/lib/libhi_mpi_sysbind.so
    ${HI3519_ROOT}/lib/libhi_mpi_sysmem.so

    # aac
    ${HI3519_ROOT}/lib/libaac_comm.so
    ${HI3519_ROOT}/lib/libaac_dec.so
    ${HI3519_ROOT}/lib/libaac_enc.so
    ${HI3519_ROOT}/lib/libaac_sbr_dec.so
    ${HI3519_ROOT}/lib/libaac_sbr_enc.so

    # opus
    ${HI3519_ROOT}/lib/libopus.so

    # math
    m
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE

    # Hi3519
    ${HI3519_ROOT}/include
    
    # internal
    ${CMAKE_CURRENT_SOURCE_DIR}/comm_audio
)

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE

    OT_ACODEC_TYPE_INNER
)