#ifndef PRINT_HPP
#define PRINT_HPP
#include <string>
#include <format>
#include <iostream>

namespace Auxil {

    template<typename T>
    concept OstreamFormattable = requires(std::ostream& os, const T& t)
    {
        { os << t } -> std::same_as<std::ostream&>;
    };

    template<typename T>
    concept Formattable = requires { typename std::formatter<T>; };

    template<typename... Args>
    std::string format(const std::string &fmt, Args&&... args) {
        return std::format(std::runtime_format(fmt), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void print(const std::string &fmt, Args&&... args) {
        auto res = Auxil::format(fmt, std::forward<Args>(args)...);
        std::cout << res;
    }

    template<typename... Args>
    void println(const std::string &fmt, Args&&... args) {
        auto res = Auxil::format(fmt, std::forward<Args>(args)...);
        std::cout << res << "\n";
    }

    template<typename... Args>
    void print(std::ostream& os, const std::string &fmt, Args&&... args) {
        auto res = Auxil::format(fmt, std::forward<Args>(args)...);
        os << res;
    }

    template<typename... Args>
    void println(std::ostream& os, const std::string &fmt, Args&&... args) {
        auto res = Auxil::format(fmt, std::forward<Args>(args)...);
        os << res << "\n";
    }


}

#endif //PRINT_HPP
