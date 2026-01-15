# CMake generated Testfile for 
# Source directory: /home/kombatant/Dev/scripts/fsearch-krunner
# Build directory: /home/kombatant/Dev/scripts/fsearch-krunner/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(appstreamtest "/usr/bin/cmake" "-DAPPSTREAMCLI=/usr/bin/appstreamcli" "-DINSTALL_FILES=/home/kombatant/Dev/scripts/fsearch-krunner/build/install_manifest.txt" "-P" "/usr/share/ECM/kde-modules/appstreamtest.cmake")
set_tests_properties(appstreamtest PROPERTIES  _BACKTRACE_TRIPLES "/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;173;add_test;/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;191;appstreamtest;/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;0;;/home/kombatant/Dev/scripts/fsearch-krunner/CMakeLists.txt;12;include;/home/kombatant/Dev/scripts/fsearch-krunner/CMakeLists.txt;0;")
