cmake_minimum_required(VERSION 3.20)

# Core library: domain + services + JSON repositories.
# Both the console and the Qt GUI link against this, so every service the GUI
# screens need must live here rather than in an executable target.
add_library(ai_interviewer_core STATIC
    # domain
    core/StrongId.cpp
    core/Difficulty.cpp
    core/Question.cpp
    core/MCQQuestion.cpp
    core/TextQuestion.cpp
    core/CodingQuestion.cpp
    core/User.cpp
    core/Student.cpp
    core/InterviewState.cpp
    core/Attempt.cpp
    core/InterviewSession.cpp
    core/QuestionSelectionStrategy.cpp
    core/LinearRampStrategy.cpp
    core/AdaptiveStrategy.cpp
    core/FeedbackGenerator.cpp
    core/RuleBasedFeedback.cpp
    core/Course.cpp
    core/IUserRepository.cpp
    core/IQuestionRepository.cpp
    core/IAttemptRepository.cpp
    core/SHA256.cpp
    # services
    auth/AuthService.cpp
    admin/AdminService.cpp
    progress/ProgressService.cpp
    course/CourseService.cpp
    interview/InterviewService.cpp
    interview/SessionFactory.cpp
    interview/InterviewSessionBuilder.cpp
    # json repositories
    db/JsonUserRepository.cpp
    db/JsonQuestionRepository.cpp
    db/JsonAttemptRepository.cpp
    db/JsonScoreboardRepository.cpp
    db/JsonDailyChallengeRepository.cpp
    db/RepositoryFactory.cpp
    ui/Leaderboard.cpp
)

target_include_directories(ai_interviewer_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

if(nlohmann_json_FOUND)
    target_link_libraries(ai_interviewer_core PUBLIC nlohmann_json::nlohmann_json)
endif()

if(fmt_FOUND)
    target_link_libraries(ai_interviewer_core PUBLIC fmt::fmt)
endif()

if(OpenSSL_FOUND)
    target_link_libraries(ai_interviewer_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

if(USE_MONGO)
    target_sources(ai_interviewer_core PRIVATE
        db/MongoUserRepository.cpp
        db/MongoQuestionRepository.cpp
        db/MongoAttemptRepository.cpp
        db/MongoInterviewRepository.cpp
        db/MongoFeedbackRepository.cpp
        db/MongoScoreboardRepository.cpp
        db/MongoDailyChallengeRepository.cpp
        db/BSONMappers.cpp
        db/Database.cpp
    )
    target_link_libraries(ai_interviewer_core PUBLIC
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
    )
    target_compile_definitions(ai_interviewer_core PUBLIC USE_MONGO)
endif()

# Console executable - JSON backend (always available)
add_executable(ai_interviewer_console
    main.cpp
    ui/ConsoleUI.cpp
)

target_link_libraries(ai_interviewer_console PRIVATE ai_interviewer_core)

# Copy the driver DLLs next to the console executable so it can run in place.
if(USE_MONGO AND MONGO_DRIVER_PREFIX)
    foreach(_dll libbson-1.0.dll libbsoncxx.dll libmongoc-1.0.dll libmongocxx.dll)
        if(EXISTS "${MONGO_DRIVER_PREFIX}/bin/${_dll}")
            add_custom_command(TARGET ai_interviewer_console POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MONGO_DRIVER_PREFIX}/bin/${_dll}"
                    "$<TARGET_FILE_DIR:ai_interviewer_console>"
                COMMENT "MongoDB driver: ${_dll}")
        endif()
    endforeach()
endif()

# GUI executable (Qt 6 Widgets) - shares the core library and services
if(BUILD_GUI AND Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)

    add_executable(ai_interviewer_gui WIN32
        gui/main_gui.cpp
        gui/SessionContext.cpp
        gui/LoginDialog.cpp
        gui/InterviewWidget.cpp
        gui/DailyChallengeWidget.cpp
        gui/ProfileWidget.cpp
        gui/LeaderboardWidget.cpp
        gui/AdminWidget.cpp
        gui/MainWindow.cpp
        # headers listed explicitly so AUTOMOC can moc them: they live in
        # include/gui, next to a matching cpp in src/gui
        ../include/gui/SessionContext.h
        ../include/gui/LoginDialog.h
        ../include/gui/InterviewWidget.h
        ../include/gui/DailyChallengeWidget.h
        ../include/gui/ProfileWidget.h
        ../include/gui/LeaderboardWidget.h
        ../include/gui/AdminWidget.h
        ../include/gui/MainWindow.h
    )

    target_include_directories(ai_interviewer_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    )

    target_link_libraries(ai_interviewer_gui PRIVATE
        ai_interviewer_core
        Qt6::Widgets
    )

    if(USE_MONGO AND MONGO_DRIVER_PREFIX)
        foreach(_dll libbson-1.0.dll libbsoncxx.dll libmongoc-1.0.dll libmongocxx.dll)
            if(EXISTS "${MONGO_DRIVER_PREFIX}/bin/${_dll}")
                add_custom_command(TARGET ai_interviewer_gui POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${MONGO_DRIVER_PREFIX}/bin/${_dll}"
                        "$<TARGET_FILE_DIR:ai_interviewer_gui>"
                    COMMENT "MongoDB driver: ${_dll}")
            endif()
        endforeach()
    endif()
endif()

# HTTP server (browser front-end over the same services)
if(BUILD_WEB)
    # cpp-httplib is header-only. OpenSSL is opt-in via
    # CPPHTTPLIB_OPENSSL_SUPPORT, which is deliberately left undefined here so
    # the server builds without OpenSSL (the project uses its own SHA-256).
    find_package(Threads REQUIRED)

    add_executable(ai_interviewer_web
        web/main_web.cpp
        web/HttpServer.cpp
        web/JsonApi.cpp
        web/SessionStore.cpp
    )

    target_include_directories(ai_interviewer_web PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
    )
    # nlohmann/json comes through ai_interviewer_core's PUBLIC include dirs when
    # found by find_package, but the vendored fallback in third_party needs the
    # explicit path above; both are covered so either layout works.



    target_link_libraries(ai_interviewer_web PRIVATE
        ai_interviewer_core
        Threads::Threads
    )
    if(WIN32)
        target_link_libraries(ai_interviewer_web PRIVATE ws2_32)
    endif()
endif()