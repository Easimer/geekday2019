set(SRC
	main.cpp

	soap.cpp
	soap.h

	level_mask.cpp
	level_mask.h

	tinyxml2.cpp
	tinyxml2.h

	${PUBLIC_DIR}/httplib.h
)

add_executable(gameclient ${SRC})
target_link_libraries(gameclient httpsrv)