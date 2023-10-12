cmake_minimum_required(VERSION 3.20)

set(MMSPARSE "mmsparser")

project(${MMSPARSE} LANGUAGES C)

add_executable(${MMSPARSE} "")

file(GLOB LIST_SRCS *.h *.c)
target_sources(
	${MMSPARSE} PRIVATE
	${LIST_SRCS}
)
