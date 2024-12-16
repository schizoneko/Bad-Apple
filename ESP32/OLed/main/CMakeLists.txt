# CMakeLists.txt for the main component

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES sh1106 fatfs sdmmc driver freertos log
)