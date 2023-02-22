cmake_minimum_required(VERSION 3.1.0)

set(PROJECT "ClusterDifferentialExpressionPlugin")

PROJECT(${PROJECT})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG /NODEFAULTLIB:LIBCMT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)
find_package(OpenMP)
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)

set(PLUGIN
    src/ClusterDifferentialExpressionPlugin.h
    src/ClusterDifferentialExpressionPlugin.cpp
   
)

set(MODEL
    src/QTableItemModel.h
    src/QTableItemModel.cpp
    src/SortFilterProxyModel.h
    src/SortFilterProxyModel.cpp
)

set(ACTIONS
    src/PluginAction.h
    src/PluginAction.cpp
    src/LoadedDatasetsAction.h
    src/LoadedDatasetsAction.cpp
    src/SettingsAction.h
    src/SettingsAction.cpp
  
)

set(WIDGETS
    src/ClusterDifferentialExpressionWidget.h
    src/ClusterDifferentialExpressionWidget.cpp
    src/WordWrapHeaderView.h
    src/WordWrapHeaderView.cpp
    src/TableView.h
    src/TableView.cpp
    src/ButtonProgressBar.h
    src/ButtonProgressBar.cpp
)

set(UTIL
    src/ProgressManager.h
    src/ProgressManager.cpp
)

set(AUX
    
    src/ClusterDifferentialExpressionPlugin.json
)

set(SOURCES ${PLUGIN} ${ACTIONS} ${MODEL} ${WIDGETS} ${UTIL})

source_group(Plugin FILES ${SOURCES})
source_group(Actions FILES ${ACTIONS})
source_group(Model FILES ${MODEL})
source_group(Widget FILES ${WIDGETS})
source_group(Util FILES ${UTIL})
source_group(Aux FILES ${AUX})

include_directories("$ENV{HDPS_INSTALL_DIR}/$<CONFIGURATION>/include/")

add_library(${PROJECT} SHARED ${SOURCES} ${UTIL} ${AUX} ${WIDGETS})

target_compile_features(${PROJECT} PRIVATE cxx_std_17)

target_link_libraries(${PROJECT} Qt6::Widgets)
target_link_libraries(${PROJECT} Qt6::WebEngineWidgets)
#target_link_libraries(${PROJECT} "$ENV{HDPS_INSTALL_DIR}/$<CONFIGURATION>/lib/ClusterData.lib")

if(MSVC)
	set(LIB_SUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")
else()
	set(LIB_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

target_link_libraries(${PROJECT} "$ENV{HDPS_INSTALL_DIR}/$<CONFIGURATION>/lib/${CMAKE_SHARED_LIBRARY_PREFIX}HDPS_Public${LIB_SUFFIX}")
target_link_libraries(${PROJECT} "$ENV{HDPS_INSTALL_DIR}/$<CONFIGURATION>/lib/${CMAKE_SHARED_LIBRARY_PREFIX}PointData${LIB_SUFFIX}")
target_link_libraries(${PROJECT} "$ENV{HDPS_INSTALL_DIR}/$<CONFIGURATION>/lib/${CMAKE_SHARED_LIBRARY_PREFIX}ClusterData${LIB_SUFFIX}")

install(TARGETS ${PROJECT}
   RUNTIME DESTINATION Plugins COMPONENT SHAREDLIB
   LIBRARY DESTINATION Plugins COMPONENT LINKLIB
   ARCHIVE DESTINATION lib COMPONENT LINKLIB
)

add_custom_command(TARGET ${PROJECT} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --prefix $ENV{HDPS_INSTALL_DIR}/$<CONFIGURATION>
)

# Automatically set the debug environment (command + working directory) for MSVC in debug mode
set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,$ENV{HDPS_INSTALL_DIR}/debug,$ENV{HDPS_INSTALL_DIR}/release>)
set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,$ENV{HDPS_INSTALL_DIR}/debug/HDPS.exe,$ENV{HDPS_INSTALL_DIR}/release/HDPS.exe>)

set_target_properties(${PROJECT} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${Qt6_DIR}/../../../bin;$<$<CONFIG:DEBUG>:$ENV{HDPS_INSTALL_DIR}/debug>; $<$<CONFIG:RELEASE>:$ENV{HDPS_INSTALL_DIR}/release>")

if(APPLE)
	MESSAGE( STATUS "Linking PREBUILT OpenMP libraries...")
	target_link_libraries("${TARGET}" debug "/usr/local/lib/libomp.dylib")
	target_link_libraries("${TARGET}" optimized "/usr/local/lib/libomp.dylib")        
endif()
if(OpenMP)
    target_link_libraries("${TARGET}" OpenMP::OpenMP_CXX)
endif()

if (OpenMP_CXX_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()


 macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

#print_all_variables()   