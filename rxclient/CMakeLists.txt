project(rxclient)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5LinguistTools)

set(SRC_FILES
    main.cpp
    network.cpp network.h
    rxclient.cpp rxclient.h
)

set(UI_FILES
    rxclient.qrc rxclient.ui
)
set(RES_FILES
    rxclient.rc resource.h rxclient.ico
)

set(TRANSLATION_FILES
    lang/zh_CN.ts
	lang/zh_TW.ts
)

qt5_create_translation(TRANSLATION_MESSAGES ${SRC_FILES} ${UI_FILES} ${TRANSLATION_FILES})
qt5_add_translation(TRANSLATION_QM ${TRANSLATION_FILES})

source_group(src FILES ${SRC_FILES})
source_group(ui FILES ${UI_FILES})
source_group(res FILES ${RES_FILES})
source_group(mocs mocs.*)
source_group(translations FILES ${TRANSLATION_FILES} ${TRANSLATION_QM})

qt5_wrap_ui(${CMAKE_CURRENT_BINARY_DIR}/ui_rxclient.h rxclient.ui)

add_executable(rxclient WIN32 ${SRC_FILES} ${UI_FILES} ${RES_FILES} ${TRANSLATION_FILES} ${TRANSLATION_QM})
target_include_directories(rxclient PRIVATE ../deps/libuv/include PRIVATE ../deps/kcp)
target_link_libraries(rxclient Qt5::Widgets uv-static kcp)

add_custom_command(TARGET rxclient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Core> $<TARGET_FILE:Qt5::Gui> $<TARGET_FILE:Qt5::Widgets> $<TARGET_FILE:Qt5::Network> $<TARGET_FILE_DIR:rxclient>
	COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:rxclient>/lang
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TRANSLATION_QM} $<TARGET_FILE_DIR:rxclient>/lang
	DEPENDS ${TRANSLATION_QM}
)
