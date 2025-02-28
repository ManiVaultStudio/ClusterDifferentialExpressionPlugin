cmake_minimum_required(VERSION 3.22)

# -----------------------------------------------------------------------------
# ClusterDifferentialExpression View Plugin
# -----------------------------------------------------------------------------
set(PROJECT "ClusterDifferentialExpressionPlugin")
PROJECT(${PROJECT})

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")    
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets Test REQUIRED)
find_package(OpenMP REQUIRED)

find_package(ManiVault COMPONENTS Core PointData ClusterData CONFIG QUIET)

if(UNIX AND NOT APPLE)
   find_package(TBB REQUIRED)
endif()

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
add_library(${PROJECT} SHARED ${SOURCES} ${UTIL} ${AUX} ${WIDGETS})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT} PRIVATE "${ManiVault_INCLUDE_DIR}")

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
target_compile_features(${PROJECT} PRIVATE cxx_std_20)

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT} PRIVATE Qt6::Widgets)
target_link_libraries(${PROJECT} PRIVATE Qt6::WebEngineWidgets)
target_link_libraries(${PROJECT} PRIVATE Qt6::Test)

target_link_libraries(${PROJECT} PRIVATE ManiVault::Core)
target_link_libraries(${PROJECT} PRIVATE ManiVault::PointData)
target_link_libraries(${PROJECT} PRIVATE ManiVault::ClusterData)

target_link_libraries(${PROJECT} PUBLIC OpenMP::OpenMP_CXX)

if(UNIX AND NOT APPLE)
   target_link_libraries(${PROJECT} PRIVATE TBB::tbb)
endif()

# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
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
        --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/Debug,$<IF:$<CONFIG:RELWITHDEBINFO>,${ManiVault_INSTALL_DIR}/RelWithDebInfo,${ManiVault_INSTALL_DIR}/Release>>)
    set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/Debug/ManiVault Studio.exe",$<IF:$<CONFIG:RELWITHDEBINFO>,"${ManiVault_INSTALL_DIR}/RelWithDebInfo/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/Release/ManiVault Studio.exe">>)
endif()
