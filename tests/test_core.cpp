// OOP: Test Suite (Google Test)
#include <gtest/gtest.h>
#include "core/StrongId.h"
#include "core/Difficulty.h"
#include "core/MCQQuestion.h"
#include "core/TextQuestion.h"
#include "core/CodingQuestion.h"
#include "core/Student.h"
#include "core/LinearRampStrategy.h"
#include "core/AdaptiveStrategy.h"
#include "core/RuleBasedFeedback.h"
#include "auth/AuthService.h"
#include "db/JsonUserRepository.h"
#include "db/JsonQuestionRepository.h"
#include <memory>

using namespace core;

// Test StrongId
TEST(StrongIdTest, GenerationAndComparison) {
    UserId id1 = UserId::generateId();
    UserId id2 = UserId::generateId();
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
    EXPECT_EQ(id1, id1);
}

// Test Difficulty
TEST(DifficultyTest, TimeLimitsAndWeights) {
    EXPECT_EQ(timeLimitMinutes(Difficulty::Easy), 1);
    EXPECT_EQ(timeLimitMinutes(Difficulty::Medium), 2);
    EXPECT_EQ(timeLimitMinutes(Difficulty::Hard), 4);
    
    EXPECT_EQ(scoreWeight(Difficulty::Easy), 1);
    EXPECT_EQ(scoreWeight(Difficulty::Medium), 2);
    EXPECT_EQ(scoreWeight(Difficulty::Hard), 3);
}

// Test MCQQuestion
TEST(MCQQuestionTest, EvaluateCorrect) {
    QuestionId id("test1");
    MCQQuestion q(id, "test", "topic", Difficulty::Easy, "Q?", {"A", "B", "C"}, 1, "Explanation");
    
    auto result = q.evaluate("B");
    EXPECT_TRUE(result.correct);
    EXPECT_DOUBLE_EQ(result.score, 1.0);
    EXPECT_NE(result.feedback.find("Correct"), std::string::npos);
}

TEST(MCQQuestionTest, EvaluateIncorrect) {
    QuestionId id("test2");
    MCQQuestion q(id, "test", "topic", Difficulty::Easy, "Q?", {"A", "B", "C"}, 1, "Explanation");
    
    auto result = q.evaluate("A");
    EXPECT_FALSE(result.correct);
    EXPECT_DOUBLE_EQ(result.score, 0.0);
    EXPECT_NE(result.feedback.find("Incorrect"), std::string::npos);
}

TEST(MCQQuestionTest, EvaluateInvalidOption) {
    QuestionId id("test3");
    MCQQuestion q(id, "test", "topic", Difficulty::Easy, "Q?", {"A", "B", "C"}, 1, "Explanation");
    
    auto result = q.evaluate("Z");
    EXPECT_FALSE(result.correct);
    EXPECT_DOUBLE_EQ(result.score, 0.0);
    EXPECT_NE(result.feedback.find("Invalid"), std::string::npos);
}

// Test TextQuestion
TEST(TextQuestionTest, EvaluateFullMatch) {
    QuestionId id("test4");
    TextQuestion q(id, "test", "topic", Difficulty::Easy, "Q?", {"keyword1", "keyword2"}, 2, "Explanation");
    
    auto result = q.evaluate("This has keyword1 and keyword2");
    EXPECT_TRUE(result.correct);
    EXPECT_DOUBLE_EQ(result.score, 1.0);
}

TEST(TextQuestionTest, EvaluatePartialMatch) {
    QuestionId id("test5");
    TextQuestion q(id, "test", "topic", Difficulty::Easy, "Q?", {"keyword1", "keyword2"}, 2, "Explanation");
    
    auto result = q.evaluate("Only keyword1 here");
    EXPECT_FALSE(result.correct);
    EXPECT_DOUBLE_EQ(result.score, 0.5);
}

TEST(TextQuestionTest, CaseInsensitive) {
    QuestionId id("test6");
    TextQuestion q(id, "test", "topic", Difficulty::Easy, "Q?", {"KEYWORD"}, 1, "Explanation");
    
    auto result = q.evaluate("keyword in lower");
    EXPECT_TRUE(result.correct);
    EXPECT_DOUBLE_EQ(result.score, 1.0);
}

// Test Student
TEST(StudentTest, StreakAndBadges) {
    UserId id("student1");
    auto student = std::make_shared<Student>(id, "test@test.com", "hash", "Test", "College", "Branch");
    
    EXPECT_EQ(student->streak(), 0);
    student->incrementStreak();
    EXPECT_EQ(student->streak(), 1);
    EXPECT_EQ(student->longestStreak(), 1);
    
    student->addBadge("First Solve");
    auto badges = student->badges();
    EXPECT_EQ(badges.size(), 1);
    EXPECT_EQ(badges[0], "First Solve");
    
    student->addBadge("First Solve"); // Duplicate
    EXPECT_EQ(student->badges().size(), 1);
}

// Test LinearRampStrategy
TEST(LinearRampStrategyTest, SelectQuestions) {
    LinearRampStrategy strategy;
    
    std::vector<QuestionPtr> pool;
    for (int i = 0; i < 5; ++i) {
        pool.push_back(std::make_shared<MCQQuestion>(
            QuestionId("e" + std::to_string(i)), "subj", "topic", Difficulty::Easy, "Q?", {"A","B"}, 0, "Exp"));
    }
    for (int i = 0; i < 5; ++i) {
        pool.push_back(std::make_shared<MCQQuestion>(
            QuestionId("m" + std::to_string(i)), "subj", "topic", Difficulty::Medium, "Q?", {"A","B"}, 0, "Exp"));
    }
    for (int i = 0; i < 5; ++i) {
        pool.push_back(std::make_shared<MCQQuestion>(
            QuestionId("h" + std::to_string(i)), "subj", "topic", Difficulty::Hard, "Q?", {"A","B"}, 0, "Exp"));
    }
    
    auto selected = strategy.selectQuestions(pool, {});
    EXPECT_EQ(selected.size(), 10);
    
    int easyCount = 0, mediumCount = 0, hardCount = 0;
    for (const auto& q : selected) {
        if (q->difficulty() == Difficulty::Easy) easyCount++;
        else if (q->difficulty() == Difficulty::Medium) mediumCount++;
        else if (q->difficulty() == Difficulty::Hard) hardCount++;
    }
    EXPECT_EQ(easyCount, 3);
    EXPECT_EQ(mediumCount, 4);
    EXPECT_EQ(hardCount, 3);
}

// Test RuleBasedFeedback
TEST(RuleBasedFeedbackTest, GenerateReport) {
    RuleBasedFeedback feedback;
    
    std::vector<Attempt> attempts;
    std::vector<QuestionPtr> questions;
    
    for (int i = 0; i < 5; ++i) {
        auto q = std::make_shared<MCQQuestion>(
            QuestionId("q" + std::to_string(i)), "subj", "topic" + std::to_string(i % 2),
            Difficulty::Easy, "Q?", {"A","B"}, 0, "Exp");
        questions.push_back(q);
        
        Attempt a;
        a.id = AttemptId("a" + std::to_string(i));
        a.userId = UserId("u1");
        a.questionId = q->id();
        a.answer = "A";
        a.correct = (i < 3);
        a.score = (i < 3) ? 1.0 : 0.0;
        a.difficulty = Difficulty::Easy;
        a.timeTakenSeconds = 30;
        a.timestamp = std::chrono::system_clock::now();
        attempts.push_back(a);
    }
    
    auto report = feedback.generate(attempts, questions);
    EXPECT_GT(report.overallScore, 0.0);
    EXPECT_LE(report.overallScore, 1.0);
    EXPECT_FALSE(report.tips.empty());
    EXPECT_LE(report.tips.size(), 3);
}

// Test AuthService hashing
TEST(AuthServiceTest, HashAndVerify) {
    auto repo = std::make_shared<JsonUserRepository>("./data/test_users.json");
    AuthService auth(repo);
    
    std::string password = "TestPass123";
    std::string hash = auth.hashPassword(password);
    
    EXPECT_TRUE(auth.verifyPassword(password, hash));
    EXPECT_FALSE(auth.verifyPassword("WrongPass", hash));
    EXPECT_NE(hash.find(':'), std::string::npos); // Contains salt separator
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}