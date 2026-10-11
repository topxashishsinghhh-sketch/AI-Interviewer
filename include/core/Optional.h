// OOP: Simple Optional implementation for GCC 6.3 compatibility
#pragma once

namespace core {

template<typename T>
class Optional {
private:
    bool hasValue_;
    T value_;
    
public:
    Optional() : hasValue_(false), value_() {}
    Optional(const T& value) : hasValue_(true), value_(value) {}
    Optional(T&& value) : hasValue_(true), value_(std::move(value)) {}
    
    bool has_value() const { return hasValue_; }
    explicit operator bool() const { return hasValue_; }
    
    T& value() { 
        if (!hasValue_) throw std::runtime_error("Optional has no value");
        return value_; 
    }
    
    const T& value() const { 
        if (!hasValue_) throw std::runtime_error("Optional has no value");
        return value_; 
    }
    
    T value_or(const T& defaultValue) const { 
        return hasValue_ ? value_ : defaultValue; 
    }
    
    T* operator->() { return &value_; }
    const T* operator->() const { return &value_; }
    T& operator*() { return value_; }
    const T& operator*() const { return value_; }
    
    void reset() { hasValue_ = false; }
    
    template<typename U>
    Optional& operator=(const U& value) {
        value_ = value;
        hasValue_ = true;
        return *this;
    }
};

} // namespace core