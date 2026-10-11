// OOP: Exception Hierarchy (Error Handling)
#pragma once
#include <stdexcept>
#include <string>

namespace db {

class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message) : std::runtime_error(message) {}
};
class ConnectionException : public DatabaseException {
public:
    explicit ConnectionException(const std::string& message) : DatabaseException("Connection error: " + message) {}
};

class QueryException : public DatabaseException {
public:
    explicit QueryException(const std::string& message) : DatabaseException("Query error: " + message) {}
};

class DuplicateKeyException : public DatabaseException {
public:
    explicit DuplicateKeyException(const std::string& message) : DatabaseException("Duplicate key: " + message) {}
};

class NotFoundException : public DatabaseException {
public:
    explicit NotFoundException(const std::string& message) : DatabaseException("Not found: " + message) {}
};

class TransactionException : public DatabaseException {
public:
    explicit TransactionException(const std::string& message) : DatabaseException("Transaction error: " + message) {}
};

// MongoDB reports a duplicate-key violation with server error code 11000
// (message contains "E11000"). mongocxx 3.x no longer exposes a write-result
// accessor on bulk_write_exception, so classify by code and message text.
inline bool isDuplicateKeyError(const std::string& what, int serverCode) {
    if (serverCode == 11000) return true;
    if (what.find("E11000") != std::string::npos) return true;
    return what.find("duplicate key") != std::string::npos;
}

} // namespace db