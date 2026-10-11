// OOP: Strategy Pattern (Concrete Strategy)
#include "core/RuleBasedFeedback.h"
#include <map>
#include <algorithm>
#include <numeric>

namespace core {

std::vector<std::string> RuleBasedFeedback::generateTips(
    const std::map<std::string, double>& scoreByTopic,
    const std::map<Difficulty, double>& scoreByDifficulty) {
    
    std::vector<std::string> tips;
    
    // Topic-based tips
    for (std::map<std::string, double>::const_iterator it = scoreByTopic.begin(); it != scoreByTopic.end(); ++it) {
        const std::string& topic = it->first;
        double score = it->second;
        if (score < 0.5) {
            tips.push_back("Review " + topic + " fundamentals - score: " + std::to_string(int(score * 100)) + "%");
        }
    }
    
    // Difficulty-based tips
    std::map<Difficulty, double>::const_iterator easyIt = scoreByDifficulty.find(Difficulty::Easy);
    std::map<Difficulty, double>::const_iterator mediumIt = scoreByDifficulty.find(Difficulty::Medium);
    std::map<Difficulty, double>::const_iterator hardIt = scoreByDifficulty.find(Difficulty::Hard);
    
    if (easyIt != scoreByDifficulty.end() && easyIt->second < 0.7) {
        tips.push_back("Focus on Easy questions to build confidence");
    }
    if (mediumIt != scoreByDifficulty.end() && mediumIt->second < 0.5) {
        tips.push_back("Practice Medium difficulty problems regularly");
    }
    if (hardIt != scoreByDifficulty.end() && hardIt->second < 0.3) {
        tips.push_back("Attempt Hard questions after mastering Medium");
    }
    
    // General tips
    if (tips.size() < 2) {
        tips.push_back("Consistent daily practice improves retention");
        tips.push_back("Review incorrect answers to identify patterns");
    }
    
    if (tips.size() > 3) tips.resize(3);
    return tips;
}

FeedbackReport RuleBasedFeedback::generate(
    const std::vector<Attempt>& attempts,
    const std::vector<QuestionPtr>& questions) const {
    
    FeedbackReport report;
    report.overallScore = 0.0;
    
    if (attempts.empty()) return report;
    
    // Build question map
    std::map<core::QuestionId, core::QuestionPtr> questionMap;
    for (std::vector<QuestionPtr>::const_iterator it = questions.begin(); it != questions.end(); ++it) {
        questionMap[(*it)->id()] = *it;
    }
    
    // Calculate scores
    double totalWeightedScore = 0.0;
    double totalWeight = 0.0;
    
    std::map<Difficulty, std::pair<double, double>> diffScores; // {score, weight}
    std::map<std::string, std::pair<double, int>> topicScores; // {score, count}
    
    for (std::vector<Attempt>::const_iterator it = attempts.begin(); it != attempts.end(); ++it) {
        const Attempt& attempt = *it;
        std::map<core::QuestionId, core::QuestionPtr>::const_iterator qIt = questionMap.find(attempt.questionId);
        if (qIt == questionMap.end()) continue;
        
        const core::QuestionPtr& question = qIt->second;
        int weight = scoreWeight(question->difficulty());
        
        totalWeightedScore += attempt.score * weight;
        totalWeight += weight;
        
        diffScores[question->difficulty()].first += attempt.score * weight;
        diffScores[question->difficulty()].second += weight;
        
        topicScores[question->topic()].first += attempt.score;
        topicScores[question->topic()].second++;
        
        if (!attempt.correct) {
            report.wrongAnswers.push_back(attempt);
        }
    }
    
    report.overallScore = totalWeight > 0 ? totalWeightedScore / totalWeight : 0.0;
    
    for (std::map<Difficulty, std::pair<double, double>>::const_iterator it = diffScores.begin(); it != diffScores.end(); ++it) {
        Difficulty diff = it->first;
        const std::pair<double, double>& pair = it->second;
        report.scoreByDifficulty[diff] = pair.second > 0 ? pair.first / pair.second : 0.0;
    }
    
    for (std::map<std::string, std::pair<double, int>>::const_iterator it = topicScores.begin(); it != topicScores.end(); ++it) {
        const std::string& topic = it->first;
        const std::pair<double, int>& pair = it->second;
        report.scoreByTopic[topic] = pair.second > 0 ? pair.first / pair.second : 0.0;
    }
    
    // Find strongest/weakest topic
    double bestScore = -1.0, worstScore = 2.0;
    for (std::map<std::string, double>::const_iterator it = report.scoreByTopic.begin(); it != report.scoreByTopic.end(); ++it) {
        const std::string& topic = it->first;
        double score = it->second;
        if (score > bestScore) {
            bestScore = score;
            report.strongestTopic = topic;
        }
        if (score < worstScore) {
            worstScore = score;
            report.weakestTopic = topic;
        }
    }
    
    report.tips = generateTips(report.scoreByTopic, report.scoreByDifficulty);
    
    return report;
}

} // namespace core