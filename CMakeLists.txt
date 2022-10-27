cmake_minimum_required(VERSION 3.19)

project(textprocessing)

add_library(textprocessing STATIC
	src/textreader.c
    src/textprocessing.c
)

target_include_directories(textprocessing PUBLIC include/)

#set_target_properties(textprocessing PROPERTIES PUBLIC_HEADERS
#    include/textprocessing/textreader.h
#    include/textprocessing/textprocessing.h
#)md


add_subdirectory(examples)