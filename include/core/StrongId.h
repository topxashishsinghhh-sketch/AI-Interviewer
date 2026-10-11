// OOP: Strong Typedef (Type Safety)
#pragma once
#include <string>
#include <functional>
#include <random>
#include <sstream>
#include <iomanip>

namespace core {

template <typename Tag, typename T = std::string>
class StrongId {
public:
    StrongId() = default;
    explicit StrongId(T value) : value_(std::move(value)) {}
    
    static StrongId generateId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        
        std::stringstream ss;
        for (int i = 0; i < 16; ++i) {
            ss << std::hex << dis(gen);
        }
        return StrongId(ss.str());
    }
    
    const T& get() const { return value_; }
    bool empty() const { return value_.empty(); }
    
    bool operator==(const StrongId& other) const { return value_ == other.value_; }
    bool operator!=(const StrongId& other) const { return value_ != other.value_; }
    bool operator<(const StrongId& other) const { return value_ < other.value_; }
    
private:
    T value_;
};

struct UserIdTag;
struct QuestionIdTag;
struct AttemptIdTag;
struct SessionIdTag;

using UserId = StrongId<UserIdTag>;
using QuestionId = StrongId<QuestionIdTag>;
using AttemptId = StrongId<AttemptIdTag>;
using SessionId = StrongId<SessionIdTag>;

} // namespace core

namespace std {
template <typename Tag, typename T>
struct hash<core::StrongId<Tag, T>> {
    size_t operator()(const core::StrongId<Tag, T>& id) const noexcept {
        return std::hash<T>{}(id.get());
    }
};
} // namespace std