#ifndef MATH_HPP
#define MATH_HPP
#include "misc.hpp"
#include "cmath"


namespace Auxil {




    template<typename T>
    concept Arithmetic = std::is_arithmetic_v<T>;

    template<typename T>
    concept BasicArithmetic = requires(T a, T b)
    {
        { a+b } -> std::same_as<T>;
        { a-b } -> std::same_as<T>;
        { a*b } -> std::same_as<T>;
        { a/b } ->std::same_as<T>;
        { a+=b } -> std::same_as<T&>;
        { a-=b } -> std::same_as<T&>;
        { a*=b } -> std::same_as<T&>;
        { a/=b } ->std::same_as<T&>;
    };


    template<Arithmetic T>
    inline constexpr T epsilon = 0;

    template<>
    inline constexpr float epsilon<float> = 1e-6f;

    template<>
    inline constexpr double epsilon<double> = 1e-15;

    template<>
    inline constexpr long double epsilon<long double> = 1e-20L;

    #if defined(__GNUC__) || defined(__clang__)

    template<>
    inline constexpr __float128 epsilon<__float128> = 1e-33;

    #endif

    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool constexpr in_range(T x, T a, T b) {
        return x > std::min(a, b) && x < std::max(a, b);
    }

    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    bool constexpr in_range_inclusive(T x, T a, T b) {
        return x >= std::min(a, b) && x <= std::max(a, b);
    }

    bool constexpr ishexdigit(const char c) {
        return isdigit(c) || in_range_inclusive(c, 'a', 'f') || in_range_inclusive(c, 'A', 'F');
    }

    bool consteval ishexdigitc(const char c) {
        return isdigit(c) || in_range_inclusive(c, 'a', 'f') || in_range_inclusive(c, 'A', 'F');
    }

    bool constexpr isbinarydigit(const char c) {
        return c == '0' || c == '1';
    }

    u8 constexpr parse_hex_digit(const char c) {
            if (!ishexdigit(c))
                throw Exception("Cannot accept non-hexadecimal digit");

            if (in_range_inclusive(c, '0', '9')) return static_cast<u8>(c - '0');
            if (in_range_inclusive(c, 'a', 'f')) return static_cast<u8>(10 + (c - 'a'));
            return static_cast<u8>(10 + (c - 'A'));
    }

    constexpr u8 BYTEMASK = 0xFF;

    struct NumericLiteralInformation {
        bool is_valid;
        union {
            u8 flags{};
            struct {
                bool is_hex: 1;
                bool is_binary: 1;
                bool negative: 1;
                u8 filler: 5;
            };
        } flags;

        NumericLiteralInformation(const bool is_valid, const bool is_hex, const bool is_binary, const bool is_negative) : is_valid(is_valid) {
            flags.is_hex = is_hex;
            flags.is_binary = is_binary;
            flags.negative = is_negative;
        }
    };

    constexpr NumericLiteralInformation analyze_literal(const std::string& s) {
        u8 flags = 0;//1 digit is hex, 2 digit is binary 4 is negative
        usize step = 0;
        if (s.empty()) return {true, false, false, false};
        if (s.front() == '-') {
            step++;
            flags |= 4;
        }
        if (s.size()-step > 2) {
            auto sub = s.substr(step, 2);
            if (sub == "0x" || sub == "0X") {
                step = 2;
                flags |= 1;
            } else if (sub == "0b" || sub == "0B") {
                step = 2;
                flags |= 2;
            }
        }
        for (; step < s.size(); step++) {
            const char c = s[step];
            if (!(
                    ((flags & 0b1) && ishexdigit(c)) ||
                    ((flags & 0b10) && isbinarydigit(c)) ||
                    (!(flags & 0b11) && isdigit(c))
                )) {
                return {false, false, false, false};
                }
        }
        return {true, static_cast<bool>(flags & 0b1), static_cast<bool>(flags & 0b10), static_cast<bool>(flags & 0b100)};
    }

    template<std::floating_point T>
    using radians_t = T;

    template<std::floating_point T>
    using arcdegrees_t = T;

    template<std::floating_point T>
    inline constexpr T TO_RADIANS = std::numbers::pi_v<T> / static_cast<T>(180);

    template<std::floating_point T>
    inline constexpr T TO_ARCDEGREES = static_cast<T>(180) / std::numbers::pi_v<T>;


    template<Arithmetic T>
    struct AngleComponents {
        T sin;
        T cos;
        T tan;
        T csc{};
        T sec{};
        T cot{};

        AngleComponents() = default;

        explicit AngleComponents(radians_t<T> theta) {
            sin = std::sin(theta);
            cos = std::cos(theta);
            tan = std::tan(theta);
            csc = static_cast<T>(1.0) / sin;
            sec = static_cast<T>(1.0) / cos;
            cot = static_cast<T>(1.0) / tan;
        }
    };

    //some common radian angles
    template<std::floating_point T>
    inline constexpr T PI_6 = std::numbers::pi_v<T> / static_cast<T>(6);

    template<std::floating_point T>
    inline constexpr T PI_4 = std::numbers::pi_v<T> / static_cast<T>(4);

    template<std::floating_point T>
    inline constexpr T PI_3 = std::numbers::pi_v<T> / static_cast<T>(3);

    template<std::floating_point T>
    inline constexpr T PI_2 = std::numbers::pi_v<T> / static_cast<T>(2);

    template<std::floating_point T>
    inline constexpr T PI2_3 = static_cast<T>(2)*std::numbers::pi_v<T> / static_cast<T>(3);

    template<std::floating_point T>
    inline constexpr T PI3_4 = static_cast<T>(3)*std::numbers::pi_v<T> / static_cast<T>(4);

    template<std::floating_point T>
    inline constexpr T PI5_6 = static_cast<T>(5)*std::numbers::pi_v<T> / static_cast<T>(6);

    template<std::floating_point T>
    inline constexpr T A_PI = std::numbers::pi_v<T>;

    template<std::floating_point T>
    inline constexpr T PI7_6 = static_cast<T>(7)*std::numbers::pi_v<T> / static_cast<T>(6);

    template<std::floating_point T>
    inline constexpr T PI5_4 = static_cast<T>(5)*std::numbers::pi_v<T> / static_cast<T>(4);

    template<std::floating_point T>
    inline constexpr T PI4_3 = static_cast<T>(4)*std::numbers::pi_v<T> / static_cast<T>(3);

    template<std::floating_point T>
    inline constexpr T PI3_2 = static_cast<T>(3)*std::numbers::pi_v<T> / static_cast<T>(2);

    template<std::floating_point T>
    inline constexpr T PI5_3 = static_cast<T>(5)*std::numbers::pi_v<T> / static_cast<T>(3);

    template<std::floating_point T>
    inline constexpr T PI7_4 = static_cast<T>(7)*std::numbers::pi_v<T> / static_cast<T>(4);

    template<std::floating_point T>
    inline constexpr T PI11_6 = static_cast<T>(11)*std::numbers::pi_v<T> / static_cast<T>(6);

    template<std::floating_point T>
    inline constexpr T PI2 = static_cast<T>(2)*std::numbers::pi_v<T>;




    template<typename T, typename A>
    concept Vector2D = std::constructible_from<T, A, A> && Arithmetic<A> &&
        requires(T vec) {
            { vec.x } -> std::convertible_to<A>;
            { vec.y } -> std::convertible_to<A>;
        };

    template<Arithmetic T>
    struct v2 {

        T x{};
        T y{};

        v2() = default;

        v2(T x, T y = T{}) : x(x), y(y) {}

        template<Vector2D<T> C>
        v2(C convert) {
            x = convert.x;
            y = convert.y;
        }

        template<Vector2D<T> C>
        FORCE_INLINE operator C() const {
            return C(x, y);
        }


        template<Arithmetic Other>
        v2& operator=(v2<Other>& other) {
            return set(other);
        }


        FORCE_INLINE friend std::ostream& operator<<(std::ostream& os, const v2& v) {
            os << "<" << v.x << ", " << v.y << ">";
            return os;
        }

        template<Arithmetic Theta = big_float>
        FORCE_INLINE static v2 of(T magnitude, radians_t<Theta> theta) {
            return {magnitude*cos(theta), magnitude*sin(theta)};
        }

        template<Arithmetic Theta = big_float>
        FORCE_INLINE static v2 of_deg(T magnitude, arcdegrees_t<Theta> theta) {
            return {magnitude*std::cos(theta*TO_RADIANS<Theta>), magnitude*std::sin(theta*TO_RADIANS<Theta>)};
        }

        template<Arithmetic Theta = big_float>
        v2 rotated(radians_t<Theta> theta, v2 origin = {0, 0}) const {
            auto t = AngleComponents<Theta>(theta);
            auto xp = (x-origin.x)*t.cos - (y-origin.y)*t.sin;
            auto yp = (x-origin.x)*t.sin + (y-origin.y)*t.cos;

            return {xp, yp};
        }

        template<Arithmetic Theta = big_float>
        v2& rotate(radians_t<Theta> theta, v2 origin = {0, 0}) {
            auto t = AngleComponents<Theta>(theta);
            const auto xp = ((x-origin.x)*t.cos - (y-origin.y)*t.sin) + origin.x;
            const auto yp = ((x-origin.x)*t.sin + (y-origin.y)*t.cos) + origin.y;

            x = xp; y = yp;

            return *this;
        }

        template<Arithmetic Theta = big_float>
        v2 rotated_deg(arcdegrees_t<Theta> theta, v2 origin = {0, 0}) const {
            auto t = AngleComponents<Theta>(theta*TO_RADIANS<Theta>);
            auto xp = (x-origin.x)*t.cos - (y-origin.y)*t.sin;
            auto yp = (x-origin.x)*t.sin + (y-origin.y)*t.cos;

            return {xp, yp};
        }

        template<Arithmetic Theta = big_float>
        v2& rotate_deg(arcdegrees_t<Theta> theta, v2 origin = {0, 0}) {
            auto t = AngleComponents<Theta>(theta*TO_RADIANS<Theta>);
            const auto xp = (x-origin.x)*t.cos - (y-origin.y)*t.sin;
            const auto yp = (x-origin.x)*t.sin + (y-origin.y)*t.cos;

            x = xp; y = yp;

            return *this;
        }

        template<Arithmetic Length = T>
        FORCE_INLINE Length length() const {
            return std::sqrt(x*x + y*y);
        }

        template<Arithmetic Length = T>
        FORCE_INLINE Length length2() const {
            return x*x + y*y;
        }

        template<Arithmetic Theta = big_float>
        FORCE_INLINE radians_t<Theta> angle() const {
            return atan2(y, x);
        }

        template<Arithmetic Theta = big_float>
        FORCE_INLINE arcdegrees_t<Theta> angle_deg() const {
            return atan2(y, x)*TO_ARCDEGREES<Theta>;
        }

        FORCE_INLINE v2 normalized(T epsilon = std::numeric_limits<T>::epsilon()) const {
            const auto l = length<T>();
            if (l <= epsilon) return {0, 0};
            return {x / l, y / l};
        }

        v2& normalize(big_float _epsilon = epsilon<big_float>) {
            auto l = length2<big_float>();
            if (l <= _epsilon) {
                x = 0;
                y = 0;
                return *this;
            }
            l = 1.0 / std::sqrt(l);

            x *= l;
            y *= l;


            return *this;
        }

        template<Arithmetic Dot = T, Arithmetic Other>
        FORCE_INLINE Dot dot(v2<Other> v) const {
            return static_cast<Dot>(x * v.x + y * v.y);
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 add(const v2<Other>& v) const {
            return v2{x+v.x, y+v.y};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 sub(const v2<Other>& v) const {
            return v2{x-v.x, y-v.y};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 add(Other v) const {
            return v2{x+v, y+v};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 sub(Other v) const {
            return v2{x-v, y-v};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 mul(const v2<Other>& v) const {
            return v2{x*v.x, y*v.y};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 div(const v2<Other> v) const {
            return v2{x/v.x, y/v.y};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 mul(Other v) const {
            return v2{x*v, y*v};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 div(Other v) const {
            return v2{x/v, y/v};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2& set(v2<Other> v) {
            x = static_cast<T>(v.x);
            y = static_cast<T>(v.y);

            return *this;
        }

        v2& reflect() {
            x = -x;
            y = -y;
            return *this;
        }

        template<Arithmetic Other>
        v2& reflect(const v2<Other>& other) {
            return set((project(other)*2).sub(*this));
        }

        FORCE_INLINE v2 reflection() const {
            return {-x, -y};
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 reflection(const v2<Other>& other) const {
            auto res = *this;
            res.reflect(other);
            return res;
        }

        template<Arithmetic Other>
        v2& project(const v2<Other>& other) {
            auto len2 = other.length2();
            return set(other*(dot(other)/len2));
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 projection(const v2<Other>& other) const {
            auto res = *this;
            res.project(other);

            return res;
        }

        FORCE_INLINE v2 operator-() const {
            return reflection();
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 operator +(const v2<Other>& other) const {
            return add(other);
        }

        template<Arithmetic Other>
        v2& operator +=(const v2<Other>& other) {
            return set(add(other));
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 operator -(const v2<Other>& other) const {
            return sub(other);
        }

        template<Arithmetic Other>
        v2& operator -=(const v2<Other>& other) {
            return set(sub(other));
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 operator +(const Other other) const {
            return add(other);
        }

        template<Arithmetic Other>
        v2& operator +=(Other other) {
            return set(add(other));
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 operator -(Other other) const {
            return sub(other);
        }

        template<Arithmetic Other>
        v2& operator -=(Other other) {
            return set(sub(other));
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 operator *(const v2<Other>& other) const {
            return mul(other);
        }

        template<Arithmetic Other>
        v2& operator *=(const v2<Other>& other) {
            return set(mul(other));
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 operator /(const v2<Other>& other) const {
            return div(other);
        }

        template<Arithmetic Other>
        v2& operator /=(const v2<Other>& other) {
            return set(div(other));
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 operator *(Other other) const {
            return mul(other);
        }

        template<Arithmetic Other>
        v2& operator *=(Other other) {
            return set(mul(other));
        }

        template<Arithmetic Other>
        FORCE_INLINE v2 operator /(Other other) const {
            return div(other);
        }

        template<Arithmetic Other>
        v2& operator /=(Other other) {
            return set(div(other));
        }


        template<Arithmetic Other>
        bool operator==(const v2<Other>& v) const {
            if constexpr (Arithmetic<Other>) {
                auto eps = epsilon<Other>;

                return std::abs(v.x - x) <= eps && std::abs(v.y - y) <= eps;
            } else if constexpr (Arithmetic<T>) {
                auto eps = epsilon<Other>;

                return std::abs(v.x - x) <= eps && std::abs(v.y - y) <= eps;
            }
            return x == v.x && y == v.y;
        }

        template<Arithmetic Other>
        FORCE_INLINE bool operator!=(const v2<Other>& v) const {
            return !operator==(v);
        }

        template<Arithmetic Other>
        std::partial_ordering operator<=>(const v2<Other>& v) const {
            auto angle1 = this->template angle<big_float>();
            auto angle2 = v.template angle<big_float>();

            auto acomparison = angle1 <=> angle2;
            if (acomparison != std::partial_ordering::equivalent) return acomparison;

            auto length2 = v.template length2<big_float>();
            auto length1 = this->template length2<big_float>();
            return length1 <=> length2;
        }
    };






    template<Arithmetic V>
    inline v2<V> V_ZERO = static_cast<V>(0);

    //common circle angles (cos, sin pairs)
    template<std::floating_point V>
    inline constexpr v2<V> V_2PI = {1, 0};

    template<std::floating_point V>
    inline constexpr v2<V> V_PI_6 = {std::cos(PI_6<V>), std::sin(PI_6<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_PI_4 = {std::cos(PI_4<V>), std::sin(PI_4<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_PI_3 = {std::cos(PI_3<V>), std::sin(PI_3<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_PI_2 = {0, 1};

    template<std::floating_point V>
    inline constexpr v2<V> V_2PI_3 = {std::cos(PI2_3<V>), std::sin(PI2_3<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_3PI_4 = {std::cos(PI3_4<V>), std::sin(PI3_4<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_5PI_6 = {std::cos(PI5_6<V>), std::sin(PI5_6<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_PI = {-1, 0};

    template<std::floating_point V>
    inline constexpr v2<V> V_7PI_6 = {std::cos(PI7_6<V>), std::sin(PI7_6<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_5PI_4 = {std::cos(PI5_4<V>), std::sin(PI5_4<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_4PI_3 = {std::cos(PI4_3<V>), std::sin(PI4_3<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_3PI_2 = {0, -1};

    template<std::floating_point V>
    inline constexpr v2<V> V_5PI_3 = {std::cos(PI5_3<V>), std::sin(PI5_3<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_7PI_4 = {std::cos(PI7_4<V>), std::sin(PI7_4<V>)};

    template<std::floating_point V>
    inline constexpr v2<V> V_11PI_6 = {std::cos(PI11_6<V>), std::sin(PI11_6<V>)};


    template<typename T, typename A>
    concept Vector3D = std::constructible_from<T, A, A, A> && Arithmetic<A> &&
        requires(T vec) {
            { vec.x } -> std::convertible_to<A>;
            { vec.y } -> std::convertible_to<A>;
            { vec.z } -> std::convertible_to<A>;
        };

    template<Arithmetic T>
    struct Quaternion;

    template<Arithmetic T>
    struct v3 {
        T x{}, y{}, z{};

        v3() = default;

        v3(T x, T y = T{}, T z = T{}) : x(x), y(y), z(z) {}

        template<Vector3D<T> C>
        v3(C convert) : x(convert.x), y(convert.y), z(convert.z) {}


        template<Arithmetic Other>
        v3& operator=(const v3<Other>& o) {
            return set(o);
        }

        template<Vector3D<T> C>
        FORCE_INLINE operator C() const {
            return {x, y, z};
        }

        FORCE_INLINE friend std::ostream& operator<<(std::ostream& os, const v3& v) {
            os << "<" << v.x << ", " << v.y << ", " << v.z << ">";

            return os;
        }

        //TODO: add static functions to construct a v3 by applying transformations to the unit x-axis vector


        //rotation by euler angles, implemented after Quaternion definition
        template<std::floating_point Theta>
        v3& rotate(const radians_t<Theta> pitch, const radians_t<Theta> yaw, const radians_t<Theta> roll);

        //rotation around a axis (should be normalized) by an angle
        template<std::floating_point Theta>
        v3& rotate(const v3& rotation_axis, radians_t<Theta> angle);

        template<Arithmetic U>
        v3& rotate(const Quaternion<U>& q);

        //rotation by euler angles, implemented after Quaternion definition
        template<std::floating_point Theta>
        v3 rotated(const radians_t<Theta> pitch, const radians_t<Theta> yaw, const radians_t<Theta> roll) {
            v3 res = *this;
            res.rotate(pitch, yaw, roll);

            return res;
        }

        //rotation around a axis (should be normalized) by an angle
        template<std::floating_point Theta>
        v3 rotated(const v3& rotation_axis, radians_t<Theta> angle) {
            v3 res = *this;
            res.rotate(rotation_axis, angle);

            return res;
        }

        template<Arithmetic U>
        v3 rotated(const Quaternion<U>& q);

        template<Arithmetic Length = T>
        FORCE_INLINE Length length() const {
            return static_cast<Length>(std::sqrt(x*x + y*y + z*z));
        }

        template<Arithmetic Length = T>
        FORCE_INLINE Length length2() const {
            return static_cast<Length>(x*x + y*y + z*z);
        }

        /*TODO: Return a transformation struct representing the transforms needed to transform the unit x-axis vector to
            this vector*/


        FORCE_INLINE v3 normalized(T _epsilon = epsilon<T>) const {
            v3 res = *this;
            res.normalize();
            return res;
        }

        v3& normalize(T _epsilon = epsilon<T>) {
            auto l = length2<T>();
            if (l <= _epsilon) return *this;
            l = 1.0/std::sqrt(l);
            x *= l;
            y *= l;
            z *= l;

            return *this;
        }

        template<Arithmetic Dot = T, Arithmetic Other>
        FORCE_INLINE Dot dot(const v3<Other>& v) const {
            return static_cast<Dot>(x * v.x + y * v.y + z * v.z);
        }

        template<Arithmetic Cross = T, Arithmetic Other>
        FORCE_INLINE v3<Cross> crossed(const v3<Other>& v) const {
            return {
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x
            };
        }

        template<Arithmetic Cross = T, Arithmetic Other>
        v3<Cross>& cross(const v3<Other>& v) {

            const T tx = y * v.z - z * v.y;
            const T ty = z * v.x - x * v.z;
            const T tz = x * v.y - y * v.x;
            x = tx;
            y = ty;
            z = tz;

            return *this;
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 add(const v3<Other>& v) const {
            return {x + v.x, y + v.y, z + v.z};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 add(const Quaternion<Other>& v) const;

        template<Arithmetic Other>
        FORCE_INLINE v3 sub(const v3<Other>& v) const {
            return {x - v.x, y - v.y, z - v.z};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 add(const Other v) const {
            return {x + v, y + v, z + v};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 sub(const Other v) const {
            return {x - v, y - v, z - v};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 sub(const Quaternion<Other>& v) const;

        template<Arithmetic Other>
        FORCE_INLINE v3 mul(const v3<Other>& v) const {
            return {x * v.x, y * v.y, z * v.z};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 div(const v3<Other>& v) const {
            return {x / v.x, y / v.y, z / v.z};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 mul(const Other v) const {
            return {x * v, y * v, z * v};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 div(const Other v) const {
            return {x / v, y / v, z / v};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 mul(const Quaternion<Other>& v) const;

        template<Arithmetic Other>
        FORCE_INLINE v3 div(const Quaternion<Other>& v) const;

        template<Arithmetic Other>
        v3& set(const v3<Other>& v) {
            x = v.x;
            y = v.y;
            z = v.z;

            return *this;
        }

        v3& reflect() {
            x = -x;
            y = -y;
            z = -z;

            return *this;
        }

        template<Arithmetic Other>
        v3& reflect(const v3<Other>& n) {
            return set(sub(n.mul(-2*dot(n))));
        }

        FORCE_INLINE v3 reflection() const {
            return {-x, -y, -z};
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 reflection(const v3<Other>& other) const {
            auto res = *this;
            res.reflect(other);
            return res;
        }

        template<Arithmetic Other>
        v3& project(const v3<Other>& n) {
            if (n.length2() <= epsilon<Other>) {
                x = y = z = 0;
                return *this;
            }
            return set(n.mul(dot(n)/n.template length2<T>()));
        }

        //assumes n is already normalized, skipping division
        template<Arithmetic Other>
        v3& fast_project(const v3<Other>& n) {
            if (n.length2() <= epsilon<Other>) {
                x = y = z = 0;
                return *this;
            }
            return set(n.mul(dot(n)));
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 projection(const v3<Other>& other) const {
            auto res = *this;
            res.project(other);

            return res;
        }

        //assumes other is normalized
        template<Arithmetic Other>
        FORCE_INLINE v3 fast_projection(const v3<Other>& other) const {
            auto res = *this;
            res.fast_project(other);

            return res;
        }

        FORCE_INLINE v3 operator-() const {
            return reflection();
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator+(const v3<Other>& o) {
            return add(o);
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator+(const Other o) {
            return add(o);
        }

        template<Arithmetic Other>
        v3& operator+=(const v3<Other>& o) {
            return set(add(o));
        }

        template<Arithmetic Other>
        v3& operator+=(const Other o) {
            return set(add(o));
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator-(const v3<Other>& o) {
            return sub(o);
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator-(const Other o) {
            return sub(o);
        }

        template<Arithmetic Other>
        v3& operator-=(const v3<Other>& o) {
            return set(sub(o));
        }

        template<Arithmetic Other>
        v3& operator-=(const Other o) {
            return set(sub(o));
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator*(const v3<Other>& o) {
            return mul(o);
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator*(const Other o) {
            return mul(o);
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator*(const Quaternion<Other> o) {
            return mul(o);
        }

        template<Arithmetic Other>
        v3& operator*=(const v3<Other>& o) {
            return set(mul(o));
        }

        template<Arithmetic Other>
        v3& operator*=(const Other o) {
            return set(mul(o));
        }

        template<Arithmetic Other>
        v3& operator*=(const Quaternion<Other> o) {
            return set(mul(o));
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator/(const v3<Other>& o) {
            return div(o);
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator/(const Other o) {
            return div(o);
        }

        template<Arithmetic Other>
        FORCE_INLINE v3 operator/(const Quaternion<Other>& o) {
            return div(o);
        }

        template<Arithmetic Other>
        v3& operator/=(const v3<Other>& o) {
            return set(div(o));
        }

        template<Arithmetic Other>
        v3& operator/=(const Other o) {
            return set(div(o));
        }

        template<Arithmetic Other>
        v3& operator/=(const Quaternion<Other> o) {
            return set(div(o));
        }

        template<Arithmetic Other>
        FORCE_INLINE bool operator==(const v3<Other>& v) {
            if constexpr (std::floating_point<Other>) {
                auto eps = epsilon<Other>;

                return std::abs(v.x - x) <= eps && std::abs(v.y - y) <= eps && std::abs(v.z - z) <= eps;
            } else if constexpr (std::floating_point<T>) {
                auto eps = epsilon<T>;

                return std::abs(v.x - x) <= eps && std::abs(v.y - y) <= eps && std::abs(v.z - z) <= eps;
            }
            return x == v.x && y == v.y && z == v.z;
        }

        template<Arithmetic Other>
        FORCE_INLINE bool operator !=(const v3<Other>& v) {
            return !this->operator==(v);
        }
    };

    template<Arithmetic T>
    inline constexpr v3<T> XAXIS = {1, 0, 0};

    template<Arithmetic T>
    inline constexpr v3<T> YAXIS = {0, 1, 0};

    template<Arithmetic T>
    inline constexpr v3<T> ZAXIS = {0, 0, 1};

    template<Arithmetic T>
    inline constexpr v3<T> NXAXIS = {-1, 0, 0};

    template<Arithmetic T>
    inline constexpr v3<T> NYAXIS = {0, -1, 0};

    template<Arithmetic T>
    inline constexpr v3<T> NZAXIS = {0, 0, -1};

    template<Arithmetic T>
    inline constexpr v3<T> ZERO = {0, 0, 0};

    //Quaternions are really useful for rotation in 3D
    template<Arithmetic T>
    struct Quaternion {
        T w, x, y, z;


        friend std::ostream& operator<<(std::ostream& os, const Quaternion& self) {
            os << self.w << (self.x < 0 ? " - ":" + ") << std::abs(self.x) << "i "
                << (self.y < 0 ? "- ":"+ ") << std::abs(self.y) << "j " << (self.z < 0 ? "- ":"+ ") << std::abs(self.z) << "k";
            return os;
        }

        FORCE_INLINE v3<T> vector_part() const {
            return {x, y, z};
        }

        template<std::floating_point Theta>
        static Quaternion make_rotator(radians_t<Theta> pitch,
                                     radians_t<Theta> yaw,
                                     radians_t<Theta> roll)
        {
            T cy = std::cos(yaw * T(0.5));
            T sy = std::sin(yaw * T(0.5));
            T cp = std::cos(pitch * T(0.5));
            T sp = std::sin(pitch * T(0.5));
            T cr = std::cos(roll * T(0.5));
            T sr = std::sin(roll * T(0.5));

            return {
                cr*cp*cy + sr*sp*sy,  // w
                sr*cp*cy - cr*sp*sy,  // x
                cr*sp*cy + sr*cp*sy,  // y
                cr*cp*sy - sr*sp*cy   // z
            };
        }

        template<std::floating_point Theta>
        static Quaternion make_rotator(const v3<T>& rotation_axis, radians_t<Theta> angle) {

            AngleComponents<Theta> components{};
            components.sin = std::sin(angle);
            components.cos = std::cos(angle);
            auto res = Quaternion{
                components.cos,
                components.sin * rotation_axis.x,
                components.sin * rotation_axis.y,
                components.sin * rotation_axis.z
            };

            res.normalize();
            return res;
        }

        template<Arithmetic Length = T>
        FORCE_INLINE Length length() const {
            return std::sqrt(w*w + x*x + y*y + z*z);
        }

        template<Arithmetic Length = T>
        FORCE_INLINE Length length2() const {
            return w*w + x*x + y*y + z*z;
        }

        template<Arithmetic Dot = T>
        FORCE_INLINE Dot dot(const Quaternion& other) const {
            return x*other.x + y * other.y + z * other.z + w * other.w;
        }

        FORCE_INLINE Quaternion conjugate() const {
            return {w, -x, -y, -z};
        }

        Quaternion inverse() const {
            auto l = length2<T>();
            if (l <= epsilon<T>) return {};
            auto res = conjugate();
            res.w /= l;
            res.x /= l;
            res.y /= l;
            res.z /= l;

            return res;
        }

        Quaternion normalized() const {
            Quaternion res = *this;
            res.normalize();
            return res;
        }

        template<Arithmetic U = T>
        Quaternion& normalize(U _epsilon = epsilon<U>) {
            auto l = length2<big_float>();
            if (l <= _epsilon) {
                w = x = y = z = 0;
                return *this;
            }
            l = 1.0 / std::sqrt(l);
            w *= l; x *= l; y *= l; z *= l;
            return *this;
        }

        //Quaternions should be unit quaternions (length == 1)
        template<Arithmetic U = T>
        static Quaternion slerp(const Quaternion& a, const Quaternion& b, U t) {
            T dot = a.dot(b);
            constexpr T DOT_THRESHOLD = T(0.9995);

            if (std::abs(dot) > DOT_THRESHOLD) {
                // If too close, fall back to linear interpolation
                Quaternion res = a + (b - a) * t;
                return res.normalized();
            }

            dot = std::clamp(dot, T(-1), T(1));
            T theta = std::acos(dot) * t;
            Quaternion relative = (b - a*dot).normalized();
            return a * std::cos(theta) + relative * std::sin(theta);
        }

        template<Arithmetic U = T>
        FORCE_INLINE Quaternion slerp(const Quaternion& other, U t) const {
            return slerp(*this, other, t);
        }


        Quaternion& operator +=(const Quaternion& other)  {
            w += other.w;
            x += other.x;
            y += other.y;
            z += other.z;

            return *this;
        }

        Quaternion operator+(const Quaternion& other) const {
            Quaternion q = *this;

            q += other;

            return q;
        }

        template<Arithmetic Other>
        Quaternion& operator +=(const v3<Other>& other)  {

            x += other.x;
            y += other.y;
            z += other.z;

            return *this;
        }

        template<Arithmetic Other>
        Quaternion operator+(const v3<Other>& other) const {
            Quaternion q = *this;

            q += other;

            return q;
        }

        template<Arithmetic Other>
        Quaternion& operator +=(const Other other)  {

            w += other;
            return *this;
        }

        template<Arithmetic Other>
        Quaternion operator+(const Other other) const {
            Quaternion q = *this;

            q += other;

            return q;
        }

        Quaternion& operator-=(const Quaternion& other)  {
            w -= other.w;
            x -= other.x;
            y -= other.y;
            z -= other.z;

            return *this;
        }

        Quaternion operator-(const Quaternion& other) const {
            Quaternion res = *this;
            res -= other;
            return *this;
        }

        template<Arithmetic Other>
        Quaternion& operator -=(const v3<Other>& other)  {

            x -= other.x;
            y -= other.y;
            z -= other.z;

            return *this;
        }

        template<Arithmetic Other>
        Quaternion operator-(const v3<Other>& other) const {
            Quaternion q = *this;

            q += other;

            return q;
        }

        template<Arithmetic Other>
        Quaternion& operator -=(const Other other)  {

            w -= other;
            return *this;
        }

        template<Arithmetic Other>
        Quaternion operator-(const Other other) const {
            Quaternion q = *this;

            q += other;

            return q;
        }

        Quaternion& operator*=(const Quaternion& q)  {
            const T nw = w*q.w - x*q.x - y*q.y - z*q.z;
            const T nx = w*q.x + x*q.w + y*q.z - z*q.y;
            const T ny = w*q.y - x*q.z + y*q.w + z*q.x;
            const T nz = w*q.z + x*q.y - y*q.x + z*q.w;
            w = nw; x = nx; y = ny; z = nz;
            return *this;
        }

        Quaternion operator*(const Quaternion& q) const {
            Quaternion res = *this;
            res *= q;
            return res;
        }

        template<Arithmetic U>
        Quaternion& operator*=(const U s)  {
            w *= s;
            x *= s;
            y *= s;
            z *= s;
            return *this;
        }


        template<Arithmetic U>
        Quaternion operator*(const U q) const {
            Quaternion res = *this;
            res *= q;
            return res;
        }

        template<Arithmetic U>
        Quaternion& operator*=(const v3<U>& q) {
            const T nw = -x*q.x - y*q.y - z*q.z;
            const T nx = w*q.x + y*q.z - z*q.y;
            const T ny = w*q.y - x*q.z + z*q.x;
            const T nz = w*q.z + x*q.y - y*q.x;
            w = nw; x = nx; y = ny; z = nz;
            return *this;
        }


        template<Arithmetic U>
        Quaternion operator*(const v3<U>& q) const {
            Quaternion res = *this;
            res *= q;
            return res;
        }

        Quaternion& operator/=(const Quaternion& q)  {
            return operator*=(q.inverse());
        }

        Quaternion operator/(const Quaternion& q) const {
            Quaternion res = *this;
            res /= q;

            return res;
        }

        template<Arithmetic U>
        Quaternion& operator/=(const U q)  {
            w /= q;
            x /= q;
            y /= q;
            z /= q;

            return *this;
        }

        template<Arithmetic U>
        Quaternion operator/(const U q) const {
            Quaternion res = *this;
            res /= q;

            return res;
        }

        template<Arithmetic U>
        Quaternion& operator/=(const v3<U>& q)  {
            x /= q.x;
            y /= q.y;
            z /= q.z;

            return *this;
        }

        template<Arithmetic U>
        Quaternion operator/(const v3<U>& q) const {
            Quaternion res = *this;
            res /= q;

            return res;
        }
    };


    template<Arithmetic T>
    inline constexpr Quaternion<T> Q_MUL_IDENTITY = {1, 0, 0, 0};

    template<Arithmetic T>
    inline constexpr Quaternion<T> Q_ZERO = {0, 0, 0, 0};

    template<Arithmetic T, Arithmetic Q>
    Quaternion<Q> operator /(const T v, const Quaternion<Q>& q) {
        if (q.length2() <= epsilon<Q>) return {0, 0, 0, 0};
        return v * q.inverse();
    }

    template<Arithmetic T>
    template<std::floating_point Theta>
    v3<T> &v3<T>::rotate(const radians_t<Theta> pitch, const radians_t<Theta> yaw, const radians_t<Theta> roll) {
        Quaternion<T> rotator = Quaternion<T>::make_rotator(pitch, yaw, roll);
        rotator = rotator * (*this) * rotator.conjugate();

        x = rotator.x;
        y = rotator.y;
        z = rotator.z;

        return *this;
    }

    template<Arithmetic T>
    template<std::floating_point Theta>
    v3<T> &v3<T>::rotate(const v3 &rotation_axis, radians_t<Theta> angle) {
        Quaternion<T> rotator = Quaternion<T>::make_rotator(rotation_axis, angle);

        rotator = rotator * (*this) * rotator.conjugate();

        x = rotator.x;
        y = rotator.y;
        z = rotator.z;

        return *this;
    }

    template<Arithmetic T>
    template<Arithmetic U>
    v3<T> &v3<T>::rotate(const Quaternion<U> &q) {
        Quaternion<T> res = q * (*this) * q.conjugate();
        x = res.x;
        y = res.y;
        z = res.z;

        return *this;
    }

    template<Arithmetic T>
    template<Arithmetic U>
    v3<T> v3<T>::rotated(const Quaternion<U> &q) {
        v3 res = *this;
        res.rotate(q);
        return res;
    }

    template<Arithmetic T>
    template<Arithmetic Other>
    v3<T> v3<T>::add(const Quaternion<Other> &v) const {
        v3 res = *this;
        res.x += v.x;
        res.y += v.y;
        res.z += v.z;


        return res;
    }

    template<Arithmetic T>
    template<Arithmetic Other>
    v3<T> v3<T>::div(const Quaternion<Other> &v) const {
        v3 res = *this;
        res.x /= v.x;
        res.y /= v.y;
        res.z /= v.z;


        return res;
    }

    template<Arithmetic T>
    template<Arithmetic Other>
    v3<T> v3<T>::mul(const Quaternion<Other> &v) const {
        v3 res = *this;
        res.x *= v.x;
        res.y *= v.y;
        res.z *= v.z;


        return res;
    }

    template<Arithmetic T>
    template<Arithmetic Other>
    v3<T> v3<T>::sub(const Quaternion<Other> &v) const {
        v3 res = *this;
        res.x -= v.x;
        res.y -= v.y;
        res.z -= v.z;


        return res;
    }








}

template<Auxil::Arithmetic T>
struct std::formatter<Auxil::v2<T>, char> {
    std::formatter<T, char> formatter_t;

    template<typename ParseContext>
    auto parse(ParseContext& ctx) {
        return formatter_t.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const Auxil::v2<T>& vec, FormatContext& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "<");

        out = formatter_t.format(vec.x, ctx);
        out = std::format_to(out, ", ");
        out = formatter_t.format(vec.y, ctx);




        return std::format_to(out, ">");
    }
};

template<Auxil::Arithmetic T>
struct std::formatter<Auxil::v3<T>, char> {
    std::formatter<T, char> formatter_t;

    template<typename ParseContext>
    auto parse(ParseContext& ctx) {
        return formatter_t.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const Auxil::v3<T>& vec, FormatContext& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "<");

        out = formatter_t.format(vec.x,  ctx);
        out = std::format_to(out, ", ");
        out = formatter_t.format(vec.y, ctx);
        out = std::format_to(out, ", ");
        out = formatter_t.format(vec.z, ctx);




        return std::format_to(out, ">");
    }
};

template<Auxil::Arithmetic T>
struct std::formatter<Auxil::Quaternion<T>, char> {
    std::formatter<T, char> formatter_t;

    template<typename ParseContext>
    auto parse(ParseContext& ctx) {
        return formatter_t.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const Auxil::Quaternion<T>& vec, FormatContext& ctx) const {

        auto out = formatter_t.format(vec.w, ctx);

        out = std::format_to(out, " {}", vec.x < 0 ? " - ":" + ");
        out = formatter_t.format(std::abs(vec.x), ctx);
        out = std::format_to(out, "i {} ", vec.y < 0 ? "-":"+");
        out = formatter_t.format(std::abs(vec.y), ctx);
        out = std::format_to(out, "j {} ", vec.z < 0 ? "-":"+");
        out = formatter_t.format(std::abs(vec.z), ctx);
        out = std::format_to(out, "j");

        return out;
    }
};



#endif