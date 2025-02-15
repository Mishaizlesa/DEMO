cmake_minimum_required(VERSION 3.0)

project(DEMO LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/src)

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCE_DIR}/src/problems.cpp)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

foreach(LAB s2_lab2)
    set(LAB_ROOT_PATH src/)
    set(LAB_SOURCES
        ${LAB_ROOT_PATH}/problems.cpp
    )

    # First executable with the first OpenBLAS path
    add_executable(main_dev_${LAB}_original ${LAB_ROOT_PATH}/main_dev.cpp ${LAB_SOURCES})
    
    target_include_directories(main_dev_${LAB}_original PUBLIC /home/mixa/newblas/opt/OpenBLAS/include)
    target_link_libraries(main_dev_${LAB}_original PRIVATE ${PROJECT_NAME} /home/mixa/newblas/opt/OpenBLAS/lib/libopenblas.a)
    target_compile_options(main_dev_${LAB}_original PUBLIC "-O3")

    # Second executable with the alternative OpenBLAS path
    add_executable(main_dev_${LAB}_alt ${LAB_ROOT_PATH}/main_dev.cpp ${LAB_SOURCES})
    
    target_include_directories(main_dev_${LAB}_alt PUBLIC /opt/OpenBLAS/include)
    target_link_libraries(main_dev_${LAB}_alt PRIVATE ${PROJECT_NAME} /opt/OpenBLAS/lib/libopenblas.a)
    target_compile_options(main_dev_${LAB}_alt PUBLIC "-O3")

endforeach()
