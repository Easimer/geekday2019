set(SRC
	main.cpp
)

add_executable(gameclient ${SRC})
target_link_libraries(gameclient httpsrv)