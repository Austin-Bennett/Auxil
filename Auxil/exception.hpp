#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#define BOOST_STACKTRACE_USE_BACKTRACE
#include "boost/stacktrace.hpp"
#include "print.hpp"

namespace Auxil {
    class Exception final : public std::runtime_error {
        boost::stacktrace::stacktrace st;
        std::string message;
    public:
        Exception() : std::runtime_error("") {
            st = boost::stacktrace::stacktrace();
            this->message.append("\n");
            message = (std::ostringstream() << st).str();
        }

        explicit Exception(const char* message) : std::runtime_error("") {
            this->message = message;
            this->message.append("\n");
            st = boost::stacktrace::stacktrace();
            this->message += (std::ostringstream() << st).str();
        }

        template<typename... Args>
        explicit Exception(const std::string& format, Args&&... args) :
        std::runtime_error("") {
            this->message.append("\n");
            message = Auxil::format(format, std::forward<Args>(args)...);
            st = boost::stacktrace::stacktrace();
            this->message += (std::ostringstream() << st).str();
        }

        [[nodiscard]] const char *what() const noexcept override {
            return message.c_str();
        }

        [[nodiscard]] boost::stacktrace::stacktrace stacktrace() {
            return st;
        }
    };
}

#endif