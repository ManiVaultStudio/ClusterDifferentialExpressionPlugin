cmake_minimum_required(VERSION 3.13.0)

set(PROJECT "ClusterDifferentialExpressionPlugin")

PROJECT(${PROJECT})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG /NODEFAULTLIB:LIBCMT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)

# Check if the directory to the ManiVault installation has been provided
if(NOT DEFINED MV_INSTALL_DIR)
    set(MV_INSTALL_DIR "" CACHE PATH "Directory where ManiVault is installed")
    message(FATAL_ERROR "Please set MV_INSTALL_DIR to the directory where ManiVault is installed")
endif()
file(TO_CMAKE_PATH ${MV_INSTALL_DIR} MV_INSTALL_DIR)

find_package(Qt6 COMPONENTS Widgets WebEngineWidgets Test REQUIRED)

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

set(PLUGIN_MOC_HEADERS
    src/ButtonProgressBar.h
    src/ClusterDifferentialExpressionPlugin.h
    src/LoadedDatasetsAction.h
    src/QTableItemModel.h
    src/SortFilterProxyModel.h
    src/TableView.h
    src/WordWrapHeaderView.h
)

add_library(${PROJECT} SHARED ${SOURCES} ${UTIL} ${AUX} ${WIDGETS})

qt_wrap_cpp(CDEPLUGIN_MOC ${PLUGIN_MOC_HEADERS} TARGET ${PROJECT})
target_sources(${PROJECT} PRIVATE ${CDEPLUGIN_MOC})

target_include_directories(${PROJECT} PRIVATE "${MV_INSTALL_DIR}/$<CONFIGURATION>/include/")

target_compile_features(${PROJECT} PRIVATE cxx_std_17)
set_property(TARGET ${PROJECT} PROPERTY CXX_STANDARD 17)

if (NOT MSVC OR MSVC_VERSION LESS 1920)
    find_package(TBB COMPONENTS tbb)
endif()
if (TBB_FOUND)
    target_link_libraries(${PROJECT} PRIVATE INTERFACE TBB::tbb)
    message("TBB version ${TBB_VERSION} interface ${TBB_INTERFACE_VERSION}")
endif()

target_link_libraries(${PROJECT} PRIVATE Qt6::Widgets)
target_link_libraries(${PROJECT} PRIVATE Qt6::WebEngineWidgets)
target_link_libraries(${PROJECT} PRIVATE Qt6::Test)

#set(MV_LINK_PATH "${MV_INSTALL_DIR}/$<CONFIGURATION>/lib")
#set(PLUGIN_LINK_PATH "${MV_INSTALL_DIR}/$<CONFIGURATION>/$<IF:$<CXX_COMPILER_ID:MSVC>,lib,Plugins>")
#set(MV_LINK_SUFFIX $<IF:$<CXX_COMPILER_ID:MSVC>,${CMAKE_LINK_LIBRARY_SUFFIX},${CMAKE_SHARED_LIBRARY_SUFFIX}>)

set(MV_LINK_LIBRARY "${MV_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}MV_Public${MV_LINK_SUFFIX}")
set(POINTDATA_LINK_LIBRARY "${PLUGIN_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}PointData${MV_LINK_SUFFIX}") 
set(CLUSTERDATA_LINK_LIBRARY "${PLUGIN_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ClusterData${MV_LINK_SUFFIX}") 

target_link_libraries(${PROJECT} PRIVATE "${MV_LINK_LIBRARY}")
target_link_libraries(${PROJECT} PRIVATE "${POINTDATA_LINK_LIBRARY}")
target_link_libraries(${PROJECT} PRIVATE "${CLUSTERDATA_LINK_LIBRARY}")

install(TARGETS ${PROJECT}
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS # Linux/Mac .so
    ARCHIVE DESTINATION lib     COMPONENT LINKLIB # Windows .lib
    PUBLIC_HEADER DESTINATION include/${PROJECT} COMPONENT HEADERS
)

add_custom_command(TARGET ${PROJECT} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --prefix ${MV_INSTALL_DIR}/$<CONFIGURATION>
)

# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${MV_INSTALL_DIR}/debug,${MV_INSTALL_DIR}/release>)
    set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${MV_INSTALL_DIR}/debug/ManiVault Studio.exe","${MV_INSTALL_DIR}/release/ManiVault Studio.exe">)

    set_target_properties(${PROJECT} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${Qt6_DIR}/../../../bin;$<$<CONFIG:DEBUG>:${MV_INSTALL_DIR}/debug>; $<$<CONFIG:RELEASE>:${MV_INSTALL_DIR}/release>")
endif()

if(APPLE)
    target_link_libraries("${PROJECT}" PUBLIC debug "/usr/local/lib/libomp.dylib")
    target_link_libraries("${PROJECT}" PUBLIC optimized "/usr/local/lib/libomp.dylib")       
endif()

find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
    message(STATUS "OpenMP found")
    target_link_libraries(${PROJECT} PUBLIC OpenMP::OpenMP_CXX)
    target_compile_options(${PROJECT} PRIVATE ${OpenMP_CXX_FLAGS})
else()
    message(WARNING "OpenMP not found!")
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