cmake_minimum_required(VERSION 3.25)

foreach(required SOURCE_ASSETS RUNTIME_ASSETS BUILD_ROOT)
    if (NOT DEFINED ${required} OR "${${required}}" STREQUAL "" OR
            NOT IS_ABSOLUTE "${${required}}")
        message(FATAL_ERROR "${required} must be an absolute path")
    endif()
endforeach()
if (NOT IS_DIRECTORY "${SOURCE_ASSETS}" OR NOT IS_DIRECTORY "${BUILD_ROOT}")
    message(FATAL_ERROR "The source assets and build root must exist")
endif()

# Resolve and check the complete destination before removing any build output.
cmake_path(NORMAL_PATH RUNTIME_ASSETS)
file(REAL_PATH "${SOURCE_ASSETS}" source)
file(REAL_PATH "${BUILD_ROOT}" build)
file(REAL_PATH "${RUNTIME_ASSETS}" destination)
cmake_path(GET destination FILENAME leaf)
set(build_key "${build}")
set(source_key "${source}")
set(destination_key "${destination}")
if (WIN32)
    string(TOLOWER "${build}" build_key)
    string(TOLOWER "${source}" source_key)
    string(TOLOWER "${destination}" destination_key)
endif()
cmake_path(IS_PREFIX build_key "${destination_key}" NORMALIZE in_build)
cmake_path(IS_PREFIX source_key "${destination_key}" NORMALIZE inside_source)
cmake_path(IS_PREFIX destination_key "${source_key}" NORMALIZE contains_source)
if (NOT in_build OR NOT leaf STREQUAL "assets" OR destination STREQUAL build OR
        inside_source OR contains_source OR NOT destination STREQUAL RUNTIME_ASSETS)
    message(FATAL_ERROR "Refusing unsafe runtime asset destination: ${RUNTIME_ASSETS}")
endif()

# Do not follow a linked source or destination, including links to another
# directory inside the build. Project content must be regular files/directories.
foreach(tree "${SOURCE_ASSETS}" "${RUNTIME_ASSETS}")
    if (IS_SYMLINK "${tree}")
        message(FATAL_ERROR "Linked asset directories are not supported: ${tree}")
    endif()
    file(GLOB_RECURSE entries LIST_DIRECTORIES true "${tree}/*")
    foreach(entry IN LISTS entries)
        if (IS_SYMLINK "${entry}")
            message(FATAL_ERROR "Linked asset entries are not supported: ${entry}")
        endif()
    endforeach()
endforeach()

# Only the verified build destination is disposable. Never write to the project.
file(REMOVE_RECURSE "${destination}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${source}" "${destination}" RESULT_VARIABLE copied)
if (NOT copied EQUAL 0)
    message(FATAL_ERROR "Failed to prepare runtime assets from ${source}")
endif()
