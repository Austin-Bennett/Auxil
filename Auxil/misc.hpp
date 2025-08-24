#ifndef MISC_HPP
#define MISC_HPP

#include "globals.hpp"

namespace Auxil {
    namespace Primitives {
        typedef uint8_t u8;
        typedef uint16_t u16;
        typedef uint32_t u32;
        typedef uint64_t u64;

        typedef int8_t i8;
        typedef int16_t i16;
        typedef int32_t i32;
        typedef int64_t i64;

        typedef size_t usize;

        typedef wchar_t wchar;

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
        typedef float f32;
        typedef double f64;
        typedef long double f80;
        typedef long double big_float;
#define ARCH_64
#else
        typedef float f32
        typedef float big_float;
#endif

        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T ston(const std::string& s) {
            T res;
            std::from_chars(s.data(), s.data() + s.size(), res);

            return res;
        }
    }


    constexpr std::string conditional_message(const bool condition, const std::string& _true, const std::string& _false) {
        return condition ? _true:_false;
    }

    std::vector<std::string> split(const std::string& s, const std::string& del = " ") {
        if (s.size() < del.size()) return {};
        if (del.empty()) return {s};
        std::vector<std::string> res;
        Primitives::usize start{0};
        for (Primitives::usize i = 0; i < s.size()-del.size()+1; i++) {
            if (s.compare(i, del.size(), del) == 0) {
                res.push_back(s.substr(start, i-start));
                i += del.size()-1;
                start = i+1;
            }
        }
        if (start < s.size()) {
            res.push_back(s.substr(start));
        }

        return res;
    }
}

#endif