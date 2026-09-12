cmake_minimum_required(VERSION 3.25)

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef unique)
set(root "${TEST_ROOT}/${unique}")
set(project "${root}/Authoring Project/assets")
set(build "${root}/build")
set(package "${build}/Debug/assets")
set(template "${root}/templates/DefaultScene.scene.json")
file(MAKE_DIRECTORY "${project}/scenes" "${project}/registry" "${build}" "${root}/templates")
file(WRITE "${template}" "original template")
file(WRITE "${project}/scenes/Main.scene.json" "first author scene")
file(WRITE "${project}/registry/runtime-assets.json" "project registry")

function(stage destination should_succeed)
    execute_process(COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_ASSETS=${project}" "-DRUNTIME_ASSETS=${destination}" "-DBUILD_ROOT=${build}"
            -P "${STAGING_SCRIPT}"
            RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if (should_succeed AND NOT result EQUAL 0)
        message(FATAL_ERROR "Staging failed: ${output}${error}")
    elseif (NOT should_succeed AND result EQUAL 0)
        message(FATAL_ERROR "Unsafe staging unexpectedly succeeded: ${destination}")
    endif()
endfunction()

function(expect_file path expected)
    file(READ "${path}" actual)
    if (NOT actual STREQUAL expected)
        message(FATAL_ERROR "Unexpected content in ${path}: ${actual}")
    endif()
endfunction()

stage("${package}" TRUE)
expect_file("${package}/scenes/Main.scene.json" "first author scene")
expect_file("${package}/registry/runtime-assets.json" "project registry")

# An asset-only edit must be packaged even when no C++ source has changed.
file(WRITE "${project}/scenes/Main.scene.json" "saved author edit")
file(WRITE "${package}/scenes/Deleted.scene.json" "stale build output")
stage("${package}" TRUE)
expect_file("${package}/scenes/Main.scene.json" "saved author edit")
expect_file("${project}/scenes/Main.scene.json" "saved author edit")
expect_file("${template}" "original template")
if (EXISTS "${package}/scenes/Deleted.scene.json")
    message(FATAL_ERROR "Stale runtime assets were not removed")
endif()

stage("${project}" FALSE)
stage("${project}/generated/assets" FALSE)
stage("${root}/outside/assets" FALSE)
stage("${build}" FALSE)
expect_file("${project}/scenes/Main.scene.json" "saved author edit")
expect_file("${template}" "original template")
message(STATUS "Runtime asset copy and authoring preservation tests passed")
