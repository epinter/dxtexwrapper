cmake_minimum_required(VERSION 3.10)

set(projectName "dxtexwrapper")
project (${projectName} VERSION 0.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(src)