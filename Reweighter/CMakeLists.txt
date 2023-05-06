cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(HTReweighter VERSION 1.0.0)

find_package(AnalysisBase REQUIRED)
atlas_project(HTReweighter 1.0.0)

find_package(ROOT 6.24 REQUIRED COMPONENTS ROOTDataFrame Tree RIO TreePlayer Hist)
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(Eigen REQUIRED)

add_executable(reweight)
target_sources(reweight PRIVATE main.cxx)
target_compile_options(reweight PRIVATE "-Wno-deprecated-copy")
target_include_directories(reweight PRIVATE ${BOOST_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} ${EIGEN_INCLUDE_DIRS} Reweighter/Reweighter/Regions.h)
target_link_libraries(reweight PRIVATE ${BOOST_LIBRARIES} ${ROOT_LIBRARIES} ${EIGEN_LIBRARIES})

lcg_generate_env(SH_FILE ${CMAKE_BINARY_DIR}/${ATLAS_PLATFORM}/env_setup.sh)
install(FILES ${CMAKE_BINARY_DIR}/${ATLAS_PLATFORM}/env_setup.sh DESTINATION .)

