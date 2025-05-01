include("/home/george/Projects/film_scanner_pc/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/korova-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE /home/george/Projects/film_scanner_pc/korova
    GENERATE_QT_CONF
)
