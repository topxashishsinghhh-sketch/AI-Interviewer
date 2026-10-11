cmake_minimum_required(VERSION 3.20)

find_package(GTest REQUIRED)

add_executable(test_core
    test_core.cpp
    ../src/core/StrongId.cpp
    ../src/core/Difficulty.cpp
    ../src/core/Question.cpp
    ../src/core/MCQQuestion.cpp
    ../src/core/TextQuestion.cpp
    ../src/core/CodingQuestion.cpp
    ../src/core/User.cpp
    ../src/core/Student.cpp
    ../src/core/InterviewState.cpp
    ../src/core/Attempt.cpp
    ../src/core/InterviewSession.cpp
    ../src/core/QuestionSelectionStrategy.cpp
    ../src/core/LinearRampStrategy.cpp
    ../src/core/AdaptiveStrategy.cpp
    ../src/core/FeedbackGenerator.cpp
    ../src/core/RuleBasedFeedback.cpp
    ../src/core/Course.cpp
    ../src/core/IUserRepository.cpp
    ../src/core/IQuestionRepository.cpp
    ../src/core/IAttemptRepository.cpp
    ../src/auth/AuthService.cpp
    ../src/course/CourseService.cpp
    ../src/interview/InterviewService.cpp
    ../src/interview/SessionFactory.cpp
    ../src/interview/InterviewSessionBuilder.cpp
    ../src/db/JsonUserRepository.cpp
    ../src/db/JsonQuestionRepository.cpp
    ../src/db/JsonAttemptRepository.cpp
    ../src/db/RepositoryFactory.cpp
)

target_include_directories(test_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(test_core PRIVATE
    GTest::GTest
    GTest::Main
    fmt::fmt
    nlohmann_json::nlohmann_json
    ssl
    crypto
)

add_test(NAME test_core COMMAND test_core)