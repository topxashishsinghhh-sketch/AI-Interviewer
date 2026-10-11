cmake_minimum_required(VERSION 3.20)

# MongoDB tools are only buildable when the USE_MONGO path is compiled into
# ai_interviewer_core, which already carries the Mongo repositories, the BSON
# mappers and the Database singleton.
if(NOT USE_MONGO)
    message(WARNING "USE_MONGO is OFF, skipping tools build")
    return()
endif()

if(NOT TARGET ai_interviewer_core)
    message(WARNING "ai_interviewer_core target missing, skipping tools build")
    return()
endif()

foreach(tool seed_db mongo_backup mongo_restore parity_check)
    add_executable(${tool} ${tool}.cpp)
    target_include_directories(${tool} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    )
    target_link_libraries(${tool} PRIVATE ai_interviewer_core)
endforeach()

# Driver DLLs must sit next to the tools as well.
if(MONGO_DRIVER_PREFIX)
    foreach(tool seed_db mongo_backup mongo_restore parity_check)
        foreach(dll libbson-1.0.dll libbsoncxx.dll libmongoc-1.0.dll libmongocxx.dll)
            if(EXISTS "${MONGO_DRIVER_PREFIX}/bin/${dll}")
                add_custom_command(TARGET ${tool} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${MONGO_DRIVER_PREFIX}/bin/${dll}"
                        "$<TARGET_FILE_DIR:${tool}>"
                    COMMENT "MongoDB driver: ${dll}")
            endif()
        endforeach()
    endforeach()
endif()
