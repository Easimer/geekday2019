set(SRC
	main.cpp

	soap.cpp
	soap.h

	level_mask.cpp
	level_mask.h

	tinyxml2.cpp
	tinyxml2.h

	entities.cpp
	entities.h

	udp.cpp
	udp.h

	${PUBLIC_DIR}/httplib.h
)

add_executable(gameclient ${SRC})
target_link_libraries(gameclient httpsrv)