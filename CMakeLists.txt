cmake_minimum_required(VERSION 3.16)

project(plasma-runner-fsearch)

set(QT_MIN_VERSION "6.0.0")
set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

# Find required packages
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Widgets
)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Runner
    I18n
    KIO
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Plugin source files
set(fsearchrunner_SRCS
    src/fsearchrunner.cpp
    src/fsearch_database_loader.cpp
)

# Create the plugin
kcoreaddons_add_plugin(krunner_fsearch
    SOURCES ${fsearchrunner_SRCS}
    INSTALL_NAMESPACE "kf6/krunner"
)

# Link libraries
target_link_libraries(krunner_fsearch
    Qt::Core
    Qt::Widgets
    KF6::Runner
    KF6::I18n
    KF6::KIOCore
    KF6::KIOWidgets
)

# Install files
install(FILES plasma-runner-fsearch.json DESTINATION ${KDE_INSTALL_DATADIR}/krunner/dbusplugins)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
