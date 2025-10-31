#ifndef STRING_HPP
#define STRING_HPP

#include <concepts>

#include "containers.hpp"
namespace Auxil {

    template<std::integral CharT>
    class BasicStr;





    template<typename T, std::integral CharT>
    constexpr BasicStr<CharT> get_typename();

    template<std::integral CharT, typename T, typename>
    constexpr BasicStr<CharT> get_typename(const T& obj);

    template<std::integral CharT>
    constexpr BasicStr<CharT> get_typename(const std::type_index& obj);


    //forward declare these so the compiler knows they exist before checking the concepts out
    template<typename T, std::integral CharT, typename = std::enable_if_t<(!std::is_same_v<T, char> && !std::is_same_v<T, wchar_t>)>>
    BasicStr<CharT> to_str(const T* ptr);

    template<Arithmetic T, typename CharT = char, typename = std::enable_if_t<sizeof(T) >= 2>>
    BasicStr<CharT> to_str(const T& value);

    template<std::integral CharT>
    BasicStr<CharT> to_str(const wchar_t& c);

    template<std::integral CharT>
    BasicStr<CharT> to_str(const signed char& c);

    template<std::integral CharT>
    BasicStr<CharT> to_str(const unsigned char& c);

    template<std::integral CharT>
    BasicStr<CharT> to_str(const char* cstr);

    template<std::integral CharT>
    BasicStr<CharT> to_str(const wchar_t* cstr);


    template<std::integral CharT>
    BasicStr<CharT> to_str(const BasicStr<wchar_t>& wstr);

    template<std::integral CharT>
    BasicStr<CharT> to_str(const BasicStr<char>& wstr);

    template<std::integral CharT>
    BasicStr<CharT> to_str(const std::string& str);

    template<std::integral CharT>
    BasicStr<CharT> to_str(const std::wstring& str);


    template<typename T, typename CharT>
    concept MemberTemplateStringifieable = requires (const T& object)
    {
      { object.template to_str<CharT>() } -> std::convertible_to<BasicStr<CharT>>;
    };

    template<typename T, typename CharT>
    concept MemberStringifieable =
    requires(const T& object)
    {
        { object.to_str() } -> std::convertible_to<BasicStr<CharT>>;
    };

    template<typename T, typename CharT>
    concept Stringifieable =
    requires(const T& object)
    {
        { to_str<CharT>(object) } -> std::convertible_to<BasicStr<CharT>>;
    };

    template<typename T, typename CharT>
    concept TemplateStringifieable =
    requires(const T& object)
    {
        { to_str<T, CharT>(object) } -> std::convertible_to<BasicStr<CharT>>;
    };

    //to_str must be callable either by passing in the char type to_str<CharT> or by the type and char type to_str<T, CharT>


    template<std::integral CharT>
    class BasicStr {
    public:
        constexpr static usize npos = std::numeric_limits<usize>::max();
    private:
        Array<CharT> _cstr{};
        usize _length{0};



        void _reserve(usize size) {
            Array<CharT> alloc(size+1);//1 to make sure we have room for the null character

            _length = std::min(size, _length);
            auto rng = _cstr | std::ranges::views::take(_length);
            std::ranges::move(rng, alloc.begin());
            alloc[_length] = CharT{};
            _cstr = std::move(alloc);
        }

        std::strong_ordering _compare(const CharT* other, usize len, usize pos = 0, usize n = npos) const {

            if (pos > len) return std::strong_ordering::less;
            n = std::min(n, len-pos);

            for (usize i = 0; i < n && i < _length; i++) {
                if (other[i+pos] != _cstr[i])
                    return other[i+pos] <=> _cstr[i];
            }

            if (n > _length) return std::strong_ordering::greater;
            if (n < _length) return std::strong_ordering::less;
            return std::strong_ordering::equal;
        }

    public:

        using value_type = CharT;
        using traits_type = std::char_traits<CharT>;
        using reference = CharT&;
        using const_reference = const CharT&;
        using pointer = CharT*;
        using const_pointer = const CharT*;
        using iterator = PointerIterator<CharT>;
        using difference_type = ptrdiff_t;
        using size_type = usize;


        BasicStr() = default;
        BasicStr(const BasicStr&) noexcept = default;
        BasicStr(BasicStr&& s) noexcept {
            _cstr = std::move(s._cstr);
            _length = s._length;
            s._length = 0;

        }


        template<typename T, typename = std::enable_if_t<!std::is_same_v<std::initializer_list<CharT>, T> && ! std::is_same_v<T, CharT*>>>
        explicit BasicStr(const T& x) {
            append(x);
        }


        BasicStr(const BasicStr& str, usize pos, usize len = npos) : _cstr(std::min(len, str._length-pos)+1),
                                    _length{_cstr.size()-1} {
            if (pos >= str._length) return;
            for (usize i = 0; i < _length; i++) {
                _cstr[i] = str._cstr[i+pos];
            }
            _cstr.back() = CharT{};
        }

        BasicStr(const CharT* s) {
            if (!s) return;
            _cstr = Array<CharT>(traits_type::length(s)+1);
            _length = _cstr.size()-1;
            for (usize i = 0; i < _length; i++) {
                _cstr[i] = s[i];
            }
            _cstr.back() = CharT{};
        }

        constexpr BasicStr(const CharT* s, size_t n) : _cstr(n+1), _length(n) {
            for (usize i = 0; i < _length; i++) {
                _cstr[i] = s[i];
            }
            _cstr.back() = CharT{};
        }

        BasicStr(const usize n, const CharT c) : _cstr(n+1), _length(n) {
            std::fill(_cstr.begin(), _cstr.begin()+_length, c);
            _cstr.back() = CharT{};
        }

        template<typename InputIterator>
        BasicStr(InputIterator first, InputIterator last) : _cstr(std::distance(first, last)+1),
                                                            _length(_cstr.size()-1) {
            std::ranges::copy(first, last, _cstr.begin());
            _cstr.back() = CharT{};
        }

        //TODO: input iterator constructor

        BasicStr(std::initializer_list<CharT> il) : _cstr(il.size()+1), _length(il.size()) {
            std::ranges::copy(il.begin(), il.end(), _cstr.begin());
            _cstr.back() = CharT{};
        }

        BasicStr& operator=(const BasicStr&) noexcept = default;
        BasicStr& operator=(BasicStr&& s) noexcept {
            if (&s == this) return *this;

            _cstr = std::move(s._cstr);
            _length = s._length;
            s._length = 0;

            return *this;
        }
        BasicStr& operator=(const char* s) {
            if (!s) {
                _cstr = Array<CharT>(1);
                _length = 0;
                _cstr[0] = CharT{};
                return *this;
            }

            _length = traits_type::length(s);
            _cstr = Array<CharT>(_length+1);

            for (usize i = 0; i < _length; i++) {
                _cstr[i] = s[i];
            }

            _cstr.back() = CharT{};
            return *this;
        }


        friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const BasicStr& s) {
            os << s._cstr.data();
            return os;
        }


        [[nodiscard]] iterator begin() const {
            return _cstr.begin();
        }

        [[nodiscard]] iterator end() const {
            return _cstr.end();
        }

        [[nodiscard]] usize size() const {
            return _length;
        }

        [[nodiscard]] usize length() const {
            return _length;
        }

        BasicStr& resize(const usize size, const char filler = ' ') {

            if (size > _length) {
                _reserve(size);
                for (usize i = _length; i < size; i++) {
                    _cstr[i] = filler;
                }
            }
            _length = size;
            _cstr[_length] = CharT{};

            return *this;
        }

        BasicStr& reserve(usize size) {
            _reserve(size);

            return *this;
        }

        BasicStr& clear() {
            if (_length == 0 || _cstr.empty()) return *this;
            _cstr[0] = CharT{};
            _length = 0;

            return *this;
        }

        [[nodiscard]] bool empty() const {
            return _length == 0;
        }

        BasicStr& shrink_to_fit() {
            _reserve(_length);
            return *this;
        }

        CharT& operator[](const usize ind) {
            if (ind >= _length) throw Exception("Cannot access character at index {}", ind);
            return _cstr[ind];
        }

        CharT& at(const usize ind) {
            if (ind >= _length) throw Exception("Cannot access character at index {}", ind);
            return _cstr[ind];
        }

        CharT& back() {
            if (_length == 0) throw Exception("Cannot access last character of empty string!");
            return _cstr[_length-1];
        }

        CharT& front() {
            if (_length == 0) throw Exception("Cannot access first character of empty string!");
            return _cstr[0];
        }


        //const versions
        [[nodiscard]] const CharT& operator[](const usize ind) const {
            if (ind >= _length) throw Exception("Cannot access character at index {}", ind);
            return _cstr[ind];
        }

        [[nodiscard]] const CharT& at(const usize ind) const {
            if (ind >= _length) throw Exception("Cannot access character at index {}", ind);
            return _cstr[ind];
        }

        [[nodiscard]] const CharT& back() const {
            if (_length == 0) throw Exception("Cannot access last character of empty string!");
            return _cstr[_length-1];
        }

        [[nodiscard]] const CharT& front() const {
            if (_length == 0) throw Exception("Cannot access first character of empty string!");
            return _cstr[0];
        }

        //this is basically the root append function that will be used in pretty much all the rest of
        //the append functions
        BasicStr& append(const CharT* cstr) {
            if (!cstr) return *this;
            auto len = traits_type::length(cstr);

            if (_length + len >= _cstr.size()) {
                //we do a exponential reserve to make frequent allocation faster,
                //and then tac on the len to make sure we have enough for the string we're about to append
                //reserve auto-handles reserving the null char
                reserve(_cstr.size()*2 + len);
            }

            std::ranges::copy(cstr, cstr+len, _cstr.begin() + _length);

            _length += len;
            _cstr[_length] = CharT{};

            return *this;
        }

        template<std::integral Char>
        BasicStr& append(const Char& c) {
            if (_length + 1 >= _cstr.size()) {
                //we do a exponential reserve to make frequent allocation faster,
                //and then tac on the len to make sure we have enough for the string we're about to append
                //reserve auto-handles reserving the null char
                reserve(_cstr.size()*2 + 1);
            }

            _cstr[_length++] = static_cast<CharT>(c);

            return *this;
        }

        BasicStr& append(const CharT* cstr, const usize len) {
            if (!cstr) return *this;


            if (_length + len >= _cstr.size()) {
                //we do a exponential reserve to make frequent allocation faster,
                //and then tac on the len to make sure we have enough for the string we're about to append
                //reserve auto-handles reserving the null char
                reserve(_cstr.size()*2 + len);
            }

            std::ranges::copy(cstr, cstr+len, _cstr.begin() + _length);

            _length += len;
            _cstr[_length] = CharT{};

            return *this;
        }

        BasicStr& append(const BasicStr& str) {
            return append(str._cstr.data(), str.size());
        }

        template<typename T>
        BasicStr& append(const T& value);


        //since strings are arrays of characters *COUGH* java
        template<std::integral Char>
        BasicStr& push_back(const Char& c) {
            if (_length + 1 >= _cstr.size()) {
                //we do a exponential reserve to make frequent allocation faster,
                //and then tac on the len to make sure we have enough for the string we're about to append
                //reserve auto-handles reserving the null char
                _reserve(_cstr.size()*2 + 1);
            }

            _cstr[_length++] = static_cast<CharT>(c);
            //null terminate
            _cstr[_length] = CharT{};

            return *this;
        }


        template<typename T>
        BasicStr& operator+=(const T& v) {
            return append(v);
        }

        template<typename T>
        BasicStr operator+(const T& v) const {
            BasicStr res = *this;
            res.append(v);
            return res;
        }


        //Ohhhhhhh boy modifiers!

        BasicStr& insert(const usize pos, const BasicStr& add) {
            if (pos >= _length) return append(add);
            if (add.empty()) return *this;

            const usize n = add.size();

            // Ensure space for new chars + null terminator
            if (_length + n + 1 > _cstr.size()) {
                _reserve(std::max(_cstr.size() * 2, _length + n + 1));
            }

            // Shift tail to the right by n, starting from the last character (not including '\0')
            for (usize i = _length; i > pos; --i) {
                _cstr[i + n - 1] = _cstr[i - 1];
            }

            // Copy the inserted string
            for (usize i = 0; i < n; ++i) {
                _cstr[pos + i] = add[i];
            }

            _length += n;

            // Set null terminator at the new end
            _cstr[_length] = CharT{};

            return *this;
        }


        void debug_print() {
            for (auto& c: _cstr) {
                if (c != CharT{}) std::cout << c;
                else std::cout << "<\\0>";
            }
            std::cout << "\n";
        }

        template<typename T, typename = std::enable_if_t<!std::is_same_v<T, BasicStr>>>
        BasicStr& insert(const usize pos, const T& stringy) {
            //convert to a string
            BasicStr r(stringy);

            return insert(pos, r);
        }

        BasicStr& erase(const usize pos, usize n = npos) {
            //this way if n is npos or something like that we won't overflow
            n = std::min(n, _length);
            if (pos >= _length) return *this;

            if (pos + n >= _length) {
                //we just chop off the string
                _length = pos;
                _cstr[_length] = CharT{};
                return *this;
            }

            //basically just move all the chars from pos+n to the end of the string back by n
            std::ranges::copy(_cstr.begin()+pos+n, _cstr.begin() + _length, _cstr.begin()+pos);

            _length -= n;
            _cstr[_length] = CharT{};
            return *this;
        }


        //replaces the characters from pos -> pos+replacement.size() with the replacement string
        BasicStr& replace(const usize pos, const BasicStr& replacement) {
            //first make sure we have enough capacity
            if (pos + replacement.size() >= _cstr.size()) {
                _reserve(_cstr.size() * 2 + replacement.size());
                _length = std::max(_length, pos + replacement.size());
            }

            for (usize i = 0; i < replacement.size(); i++) {
                _cstr[i+pos] = replacement[i];
            }
            _cstr[_length] = CharT{};

            return *this;
        }

        //replaces the characters from pos -> pos+min(replacement.size(), n) with the replacement string
        BasicStr& replace(const usize pos, usize n, const BasicStr& replacement) {
            //first make sure we have enough capacity
            n = std::min(replacement.size(), n);
            if (pos + n >= _cstr.size()) {
                _reserve(_cstr.size() * 2 + n);
                _length = std::max(_length, pos + n);
            }

            for (usize i = 0; i < n; i++) {
                _cstr[i+pos] = replacement[i];
            }

            _cstr[_length] = CharT{};

            return *this;
        }

        template<typename T, typename = std::enable_if_t<!std::is_same_v<T, BasicStr>>>
        BasicStr& replace(const usize pos, const T& obj) {
            BasicStr res;
            res.append(obj);
            return replace(pos, res);
        }

        template<typename T, typename = std::enable_if_t<!std::is_same_v<T, BasicStr>>>
        BasicStr& replace(const usize pos, const usize n, const T& obj) {
            BasicStr res;
            res.append(obj);
            return replace(pos, n, res);
        }

        //replaces the characters from pos -> pos+n with the replacement string and if n > the size, shifts the characters back
        BasicStr& replace_exactly(const usize pos, usize n, const BasicStr& replacement) {
            if (pos >= _length) return *this;
            n = std::min(n, _length - pos);

            const usize rlen = replacement.size();

            // Ensure capacity for potential growth
            if (_length + (rlen > n ? rlen - n : 0) + 1 > _cstr.size()) {
                _reserve(std::max(_cstr.size() * 2, _length + (rlen > n ? rlen - n : 0) + 1));
            }

            if (rlen > n) {
                // Shift tail right to make room
                for (usize i = _length; i > pos + n; --i) {
                    _cstr[i + (rlen - n) - 1] = _cstr[i - 1];
                }
            } else if (rlen < n) {
                // Shift tail left to remove extra space
                for (usize i = pos + n; i < _length; ++i) {
                    _cstr[i - (n - rlen)] = _cstr[i];
                }
            }

            // Copy replacement into position
            for (usize i = 0; i < rlen; ++i) {
                _cstr[pos + i] = replacement[i];
            }

            _length = _length + rlen - n;
            _cstr[_length] = CharT{}; // null terminator

            return *this;
        }


        template<typename T, typename = std::enable_if_t<!std::is_same_v<T, BasicStr>>>
        BasicStr& replace_exactly(const usize pos, const usize n, const T& obj) {
            BasicStr res;
            res.append(obj);
            return replace_exactly(pos, n, res);
        }


        CharT pop_back() {
            if (!_length) throw Exception("Cannot pop back of empty string!");

            CharT res = _cstr[_length-1];
            _length--;
            _cstr[_length] = CharT{};

            return res;
        }

        bool try_pop_back(CharT& out) {
            if (!_length) return false;

            CharT res = _cstr[_length-1];
            _length--;
            _cstr[_length] = CharT{};

            out = res;
            return true;
        }

        CharT pop_front() {
            if (!_length) throw Exception("Cannot pop front of empty string!");

            CharT res = _cstr[0];
            erase(0, 1);

            return res;
        }

        bool try_pop_front(CharT& out) {
            if (!_length) return false;

            CharT res = _cstr[0];

            erase(0, 1);

            out = res;
            return true;
        }

        usize count(const BasicStr& str) const {
            if (str.empty() || str.size() > _length) return 0;
            if (_length == 0) return 0;
            usize c = 0;

            for (usize i = 0; i <= _length-str.size(); i++) {
                if (str.compare(*this, i, str.size()) == std::strong_ordering::equal) c++;
            }

            return c;
        }

        BasicStr substr(const usize pos, usize n = npos) const {
            if (pos >= _length) throw Exception("Cannot create substring from slice {}..{} of string \"{}\"", pos, pos+n,
                BasicStr<char>(_cstr.data()));
            n = std::min(n, _length-pos);
            BasicStr res;
            res.reserve(n);

            for (usize i = 0; i < n; i++) {
                res.push_back(_cstr[i+pos]);
            }

            return res;
        }

        [[nodiscard]] std::vector<BasicStr> split(const BasicStr& delimiter) const {
            if (delimiter.empty() || delimiter.size() > _length || empty()) return {*this};
            std::vector<BasicStr> result;
            usize start = 0;

            for (usize i = 0; i <= _length-delimiter.size(); i++) {
                if (delimiter.compare(*this, i, delimiter.size()) == std::strong_ordering::equal) {
                    if (i != start) {
                        result.push_back(substr(start, i-start));
                    }
                    start = i + delimiter.size();
                    i = start-1;
                }
            }

            if (start < _length) {
                result.push_back(substr(start));
            }

            return result;
        }


        template<class It>
        It split(const BasicStr& delimiter, It _start, It _end) const {
            if (_start == _end) return _start;
            if (delimiter.empty() || delimiter.size() > _length || empty()) {
                *_start = *this;
                return ++_start;
            }

            It cur = _start;
            usize start = 0;

            for (usize i = 0; i <= _length-delimiter.size() && cur != _end; i++) {
                if (delimiter.compare(*this, i, delimiter.size()) == std::strong_ordering::equal) {
                    if (i != start) {
                        *cur = substr(start, i-start);
                        ++cur;
                    }
                    start = i + delimiter.size();
                    i = start-1;//++ makes it start next iteration
                }
            }

            if (start < _length && cur != _end) {
                *cur = substr(start);
                ++cur;
            }

            return cur;
        }

        template<typename T, typename = std::enable_if_t<!std::is_same_v<T, BasicStr>>>
        [[nodiscard]] std::vector<BasicStr> split(const T& delimiter) const {
            BasicStr del;
            del.append(delimiter);
            return split(del);
        }

        template<typename T, class It, typename = std::enable_if_t<!std::is_same_v<T, BasicStr>>>
        [[nodiscard]] It split(const T& delimiter, It start, It end) const {
            BasicStr del;
            del.append(delimiter);
            return split(del, start, end);
        }



        template<Functor<usize, const BasicStr&, usize, bool> T>
        [[nodiscard]] std::vector<BasicStr> split_if(T&& pred, bool keep_skipped = false) const {
            std::vector<BasicStr> result;
            usize start = 0;
            bool just_split = false;

            for (usize i = 0; i < _length; i++) {
                if (usize skip = pred(*this, i, just_split)) {
                    just_split = true;
                    skip = std::min(skip, _length-i);
                    if (i != start) {
                        result.push_back(substr(start, i-start));
                    }
                    if (keep_skipped) {
                        result.push_back(substr(i, skip));
                    }
                    start = i + skip;
                    i = start-1;
                } else just_split = false;
            }

            if (start < _length) {
                result.push_back(substr(start));
            }

            return result;
        }

        template<Functor<usize, const BasicStr&, usize> T, class It>
        [[nodiscard]] It split_if(It _start, It _end, T&& pred, bool keep_skipped = false) const {
            if (_start == _end) return _start;
            usize start = 0;

            for (usize i = 0; i < _length && _start != _end; i++) {
                if (usize skip = pred(*this, i)) {
                    skip = std::min(skip, _length-i);
                    if (i != start) {
                        *_start = substr(start, i-start);
                        ++_start;
                    }
                    if (keep_skipped && _start != _end) {
                        *_start = substr(i, skip);
                        ++_start;
                    }
                    start = i + skip;
                    i = start-1;
                }
            }

            if (start < _length && _start != _end) {
                *_start = substr(start);
                ++_start;
            }

            return _start;
        }

        BasicStr& trim() {
            usize n = 0;
            while (n < _length && std::iswspace(static_cast<unsigned char>(_cstr[n]))) ++n;
            if (n != 0) erase(0, n);

            usize back = _length;
            while (back > 0 && std::iswspace(static_cast<unsigned char>(_cstr[back-1]))) --back;
            if (back != _length) {
                erase(back - (back != 0));
            }

            return *this;
        }

        [[nodiscard]] BasicStr trimmed() const {
            BasicStr res = *this;
            res.trim();

            return res;
        }

        BasicStr& lower() {
            //converts all alphabetical characters to lowercase if needed
            auto loc = std::locale();
            for (CharT& c: _cstr) {
                if (std::isalpha(c, loc) && std::isupper(c, loc)) {
                    c = std::tolower(c, loc);
                }
            }

            return *this;
        }

        [[nodiscard]] BasicStr lowered() const {
            BasicStr res = *this;
            res.lower();
            return res;
        }


        BasicStr& upper() {
            //converts all alphabetical characters to lowercase if needed
            auto loc = std::locale();
            for (CharT& c: _cstr) {
                if (std::isalpha(c, loc) && std::islower(c, loc)) {
                    c = std::toupper(c, loc);
                }
            }

            return *this;
        }

        [[nodiscard]] BasicStr uppered() const {
            BasicStr res = *this;
            res.upper();
            return res;
        }

        [[nodiscard]] iterator find(const BasicStr& s, usize pos = 0, usize n = npos) const {
            if (s.size() > _length) return end();
            if (pos >= _length) return end();
            n = std::min(n, _length-pos);

            for (usize i = 0; i <= n-s.size(); i++) {
                if (s.compare(*this, i+pos, s.size()) == std::strong_ordering::equal) {
                    return begin()+pos+i;
                }
            }
            return end();
        }

        [[nodiscard]] iterator rfind(const BasicStr& s, usize pos = 0, usize n = npos) const {
            if (s.size() > _length) return end();
            if (pos >= _length) return end();
            n = std::min(n, _length-pos);
            //only difference is we check in reverse, and use i-1 to guard against underflow
            for (usize i = n-s.size()+1; i > 0 ; --i) {
                if (s.compare(*this, i+pos-1, s.size()) == std::strong_ordering::equal) {
                    return begin()+pos+i-1;
                }
            }
            return end();
        }

        [[nodiscard]] usize index(const BasicStr& s, usize pos = 0, usize n = npos) const {
            if (s.size() > _length) return npos;
            if (pos >= _length) return npos;
            n = std::min(n, _length-pos);

            for (usize i = 0; i <= n-s.size(); i++) {
                if (s.compare(*this, i+pos, s.size()) == std::strong_ordering::equal) {
                    return pos+i;
                }
            }
            return npos;
        }

        [[nodiscard]] usize rindex(const BasicStr& s, usize pos = 0, usize n = npos) const {
            if (s.size() > _length) return npos;
            if (pos >= _length) return npos;
            n = std::min(n, _length-pos);
            //only difference is we check in reverse, and use i-1 to guard against underflow
            for (usize i = n-s.size()+1; i > 0 ; --i) {
                if (s.compare(*this, i+pos-1, s.size()) == std::strong_ordering::equal) {
                    return pos+i-1;
                }
            }
            return npos;
        }

        template<typename T, std::enable_if_t<!std::same_as<BasicStr, T>>>
        iterator find(const T& x, usize pos = 0, usize n = npos) const {
            return find(BasicStr(x), pos, n);
        }

        template<typename T, std::enable_if_t<!std::same_as<BasicStr, T>>>
        iterator rfind(const T& x, usize pos = 0, usize n = npos) const {
            return rfind(BasicStr(x), pos, n);
        }


        [[nodiscard]] bool starts_with(const BasicStr& s) const {
            if (s.size() > _length) return false;
            return s.compare(*this, 0, s.size()) == std::strong_ordering::equal;
        }

        [[nodiscard]] bool starts_with(const CharT* s) const {
            auto len = traits_type::length(s);
            if (len > _length) return false;
            for (usize i = 0; i < _length && i < len; i++) {
                if (s[i] != _cstr[i]) return false;
            }
            return true;
        }

        [[nodiscard]] bool starts_with(const CharT* s, const usize len) const {
            if (len > _length) return false;
            for (usize i = 0; i < _length && i < len; i++) {
                if (s[i] != _cstr[i]) return false;
            }
            return true;
        }

        [[nodiscard]] bool ends_with(const BasicStr& s) const {
            if (s.size() > _length) return false;
            return s.compare(*this, _length-s.size(), s.size()) == std::strong_ordering::equal;
        }

        template<typename... Args>
        [[nodiscard]] BasicStr format(Args&&... args) const {
            auto res = std::format(std::runtime_format(_cstr.data()), std::forward<Args>(args)...);
            return BasicStr(res.begin(), res.end());
        }



        [[nodiscard]] bool is_alphabetical() const {
            if (empty()) return false;
            auto loc = std::locale();

            for (usize i = 0; i < _length; i++) {
                if (!std::isalpha(_cstr[i], loc)) return false;
            }
            return true;
        }

        [[nodiscard]] bool is_alphanumeric() const {
            if (empty()) return false;
            auto loc = std::locale();

            for (usize i = 0; i < _length; i++) {
                if (!std::isalnum(_cstr[i], loc)) return false;
            }
            return true;
        }

        [[nodiscard]] bool is_numeric() const {
            if (empty()) return false;
            auto loc = std::locale();
            usize i = 0;
            u32 num_periods = 0;
            if (_cstr[i] == CharT{'-'}) i++;
            for (;i < _length; i++) {
                if (!std::isdigit(_cstr[i], loc) && _cstr[i] != CharT{'.'}) return false;
                if (_cstr[i] == CharT{'.'}) num_periods++;
            }
            return num_periods <= 1;
        }


        const CharT* c_str() const {
            return _cstr.data();
        }

        CharT* data() {
            return _cstr.data();
        }

        //Comparators

        //compares with the other strings substring from pos to pos+n
        std::strong_ordering compare(const BasicStr& other, usize pos = 0, usize n = npos) const {
            if (pos > other.length()) return std::strong_ordering::less;
            n = std::min(n, other.length()-pos);

            for (usize i = 0; i < n && i < _length; i++) {
                if (other[i+pos] != _cstr[i])
                    return _cstr[i] <=> other[i+pos];
            }

            if (n > _length) return std::strong_ordering::greater;
            if (n < _length) return std::strong_ordering::less;
            return std::strong_ordering::equal;
        }

        std::strong_ordering compare_ignore_case(const BasicStr& other, usize pos = 0, usize n = npos) const {
            if (pos > other.length()) return std::strong_ordering::less;
            n = std::min(n, other.length()-pos);

            for (usize i = 0; i < n && i < _length; i++) {
                if (std::tolower((int)other[i+pos]) != std::tolower((int)_cstr[i]))
                    return std::tolower(_cstr[i]) <=> std::tolower(other[i+pos]);
            }

            if (n > _length) return std::strong_ordering::greater;
            if (n < _length) return std::strong_ordering::less;
            return std::strong_ordering::equal;
        }


        std::strong_ordering compare(const CharT* other, usize pos = 0, usize n = npos) const {
            auto olength = traits_type::length(other);
            if (pos > olength) return std::strong_ordering::less;
            n = std::min(n, olength-pos);

            for (usize i = 0; i < n && i < _length; i++) {
                if (other[i+pos] != _cstr[i])
                    return other[i+pos] <=> _cstr[i];
            }

            if (n > _length) return std::strong_ordering::greater;
            if (n < _length) return std::strong_ordering::less;
            return std::strong_ordering::equal;
        }

        std::strong_ordering compare_ignore_case(const CharT* other, usize pos = 0, usize n = npos) const {
            auto olength = traits_type::length(other);
            if (pos > olength) return std::strong_ordering::less;
            n = std::min(n, olength-pos);

            for (usize i = 0; i < n && i < _length; i++) {
                if (std::tolower((int)other[i+pos]) != std::tolower((int)_cstr[i]))
                    return std::tolower(_cstr[i]) <=> std::tolower(other[i+pos]);
            }

            if (n > _length) return std::strong_ordering::greater;
            if (n < _length) return std::strong_ordering::less;
            return std::strong_ordering::equal;
        }

        //relational operators
        bool operator==(const BasicStr &str) const {
            return compare(str) == std::strong_ordering::equal;
        }

        bool operator!=(const BasicStr &str) const {
            return compare(str) != std::strong_ordering::equal;
        }

        bool operator>(const BasicStr& str) const {
            return compare(str) == std::strong_ordering::greater;
        }

        bool operator<(const BasicStr& str) const {
            return compare(str) == std::strong_ordering::less;
        }

        bool operator>=(const BasicStr& str) const {
            auto c = compare(str);
            return c == std::strong_ordering::greater || c == std::strong_ordering::less;
        }

        bool operator<=(const BasicStr& str) const {
            return compare(str) == std::strong_ordering::less;
        }

        //non member stuff
        friend BasicStr operator +(const CharT* chars, const BasicStr& str) {
            auto res = BasicStr(chars);
            res.append(str);
            return res;
        }


        friend std::basic_istream<CharT, traits_type>&
        operator>>(std::basic_istream<CharT, traits_type>& is, BasicStr& str) {
            using traits_t = traits_type;
            auto const& ctype = std::use_facet<std::ctype<CharT>>(is.getloc());


            // Skip leading whitespace
            typename traits_t::int_type c = is.peek();
            while (!traits_t::eq_int_type(c, traits_t::eof()) &&
                   ctype.is(std::ctype_base::space, traits_t::to_char_type(c)))
            {
                is.get();
                c = is.peek();
            }

            if (traits_t::eq_int_type(c, traits_t::eof())) {
                is.setstate(std::ios_base::failbit);
                return is;
            }

            str.clear();

            // Extract characters until next whitespace
            while (!traits_t::eq_int_type(c, traits_t::eof()) &&
                   !ctype.is(std::ctype_base::space, traits_t::to_char_type(c)))
            {
                str.push_back(traits_t::to_char_type(is.get()));
                c = is.peek();
            }

            return is;
        }

    };

    //integers
    template<std::integral T, typename = std::enable_if_t<!std::is_same_v<bool, T>>>
    T ston(const BasicStr<char>& str, int base) {
        T result{};
        auto [ptr, ec] = std::from_chars(str.c_str(), str.c_str() + str.size(), result, base);
        if (ec != std::errc{}) {
            throw Exception("Invalid numeric string: {}\n"
                            "Note: with base = {}",str, base);
        }
        return result;
    }

    template<std::integral T, typename = std::enable_if_t<!std::is_same_v<bool, T>>>
    T ston(const BasicStr<char>& str) {
        return ston<T>(str, 10);
    }

    //floats
    template<std::floating_point T, typename = std::enable_if_t<!std::is_same_v<bool, T>>>
    T ston(const BasicStr<char>& str, std::chars_format fmt) {
        T result{};
        auto [ptr, ec] = std::from_chars(str.c_str(), str.c_str() + str.size(), result, fmt);
        if (ec != std::errc{}) {
            throw Exception("Invalid numeric string: {}", str);
        }
        return result;
    }

    template<std::floating_point T, typename = std::enable_if_t<!std::is_same_v<bool, T>>>
    T ston(const BasicStr<char>& str) {
        return ston<T>(str, std::chars_format::general);
    }

    //bool
    inline bool stob(const BasicStr<char>& str) {
        if (str.compare_ignore_case("true") == std::strong_ordering::equal) return true;
        if (str.is_numeric() && ston<f64>(str) != 0) return true;
        return false;
    }

    //wide integers
    template<std::integral T, typename = std::enable_if_t<!std::is_same_v<bool, T>>>
    T ston(const BasicStr<wchar_t>& str, int base) {
        return ston<T>(BasicStr<char>(str), base);
    }

    template<std::integral T, typename = std::enable_if_t<!std::is_same_v<bool, T>>>
    T ston(const BasicStr<wchar_t>& str) {
        return ston<T>(BasicStr<char>(str));
    }

    //wide floats
    template<std::floating_point T, typename = std::enable_if_t<!std::is_same_v<bool, T>>>
    T ston(const BasicStr<wchar_t>& str, std::chars_format fmt) {
        return ston<T>(BasicStr<char>(str), fmt);
    }

    template<std::floating_point T, typename = std::enable_if_t<!std::is_same_v<bool, T>>>
    T ston(const BasicStr<wchar_t>& str) {
        return ston<T>(BasicStr<char>(str), std::chars_format::general);
    }

    //wide bool
    inline bool stob(const BasicStr<wchar_t>& str) {
        if (str.compare_ignore_case(L"true") == std::strong_ordering::equal) return true;
        if (str.is_numeric() && ston<f64>(str) != 0) return true;
        return str.length() > 0;
    }


    //
    // template<std::integral CharT, std::floating_point T>
    // T ston(const BasicStr<CharT>& str, int base = 10) {
    //     long double result = std::wcstold(str.data(), nullptr);
    //
    //     return static_cast<T>(result);
    // }


    //char stuff
    template<std::integral CharT>
    BasicStr<CharT> to_str(const unsigned char& c) {
        auto c2 = static_cast<CharT>(c);
        return BasicStr<CharT>(&c2, 1);
    }

    template<std::integral CharT>
    BasicStr<CharT> to_str(const signed char& c) {
        auto c2 = static_cast<CharT>(c);
        return BasicStr<CharT>(&c2, 1);
    }

    template<std::integral CharT>
    BasicStr<CharT> to_str(const wchar_t& c) {
        auto c2 = static_cast<CharT>(c);
        return BasicStr<CharT>(&c2, 1);
    }

    template<Arithmetic T, typename CharT, typename>
    BasicStr<CharT> to_str(const T& value) {
        char buffer[32];
        auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
        assert(ec == std::errc());
        return BasicStr<CharT>(buffer, ptr);
    }

    template<typename T, std::integral CharT, typename>
    BasicStr<CharT> to_str(const T *ptr) {
        if (!ptr) {
            auto res = "0x0";

            return {res, res+3};
        } // optional, handle null pointer

        auto address = reinterpret_cast<uintptr_t>(ptr);

        char buffer[2 + sizeof(uintptr_t) * 2 + 1]; // "0x" + 2 chars per byte + null
        buffer[0] = '0';
        buffer[1] = 'x';

        auto [ptr_end, ec] = std::to_chars(buffer + 2, buffer + sizeof(buffer), address, 16);
        assert(ec == std::errc());

        return {buffer, ptr_end};
    }



    template<std::integral CharT>
    BasicStr<CharT> to_str(const char *cstr) {
        return BasicStr<CharT>(cstr, cstr+std::char_traits<char>::length(cstr));
    }

    template<std::integral CharT>
    BasicStr<CharT> to_str(const wchar_t *cstr) {
        return BasicStr<CharT>(cstr, cstr+std::char_traits<wchar_t>::length(cstr));
    }

    template<std::integral CharT>
    BasicStr<CharT> to_str(const BasicStr<wchar_t>& wstr) {
        return {wstr.begin(), wstr.end()};
    }

    template<std::integral CharT>
    BasicStr<CharT> to_str(const BasicStr<char>& str) {
        return {str.begin(), str.end()};
    }

    template<std::integral CharT>
    BasicStr<CharT> to_str(const std::string &str) {
        return {str.begin(), str.end()};
    }

    template<std::integral CharT>
    BasicStr<CharT> to_str(const std::wstring &str) {
        return {str.begin(), str.end()};
    }



    template<typename T, std::integral CharT>
    constexpr BasicStr<CharT> get_typename() {
        int status;
        char* demangle = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);

        const char* s = (status == 0 ? demangle : typeid(T).name());
        BasicStr<CharT> result(s, s+std::char_traits<char>::length(s));

        // Free the demangled string if it's not null
        if (demangle) std::free(demangle);

        return result;
    }

    template<std::integral CharT, typename T, typename = std::enable_if_t<!std::is_same_v<T, std::type_index>>>
    constexpr BasicStr<CharT> get_typename([[maybe_unused]] const T& obj) {
        int status;
        char* demangle = abi::__cxa_demangle(typeid(obj).name(), nullptr, nullptr, &status);

        const char* s = (status == 0 ? demangle : typeid(obj).name());
        BasicStr<CharT> result(s, s+std::char_traits<char>::length(s));

        // Free the demangled string if it's not null
        if (demangle) std::free(demangle);

        return result;
    }

    template<std::integral CharT>
    constexpr BasicStr<CharT> get_typename(const std::type_index& ti) {
        int status;
        char* demangle = abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status);

        const char* s = (status == 0 ? demangle : ti.name());
        BasicStr<CharT> result(s, s+std::char_traits<char>::length(s));

        // Free the demangled string if it's not null
        if (demangle) std::free(demangle);

        return result;
    }

    template<std::integral CharT>
    template<typename T>
    BasicStr<CharT> &BasicStr<CharT>::append(const T &value) {
        BasicStr appendage;
        if constexpr (Stringifieable<T, CharT>) {
            appendage = to_str<CharT>(value);

        } else if constexpr (TemplateStringifieable<T, CharT>) {
            appendage = to_str<T, CharT>(value);

        } else if constexpr (MemberTemplateStringifieable<T, CharT>) {
            appendage = value.template to_str<CharT>();

        } else if constexpr (MemberStringifieable<T, CharT>) {
            appendage = value.to_str();

        } else {
            appendage = get_typename<CharT>(value);
            appendage.append(':').append(to_str<T, CharT>(&value));
        }

        return append(appendage);
    }





    using str = BasicStr<char>;
    using wstr = BasicStr<wchar_t>;
    using strmatch = std::match_results<str::iterator>;
    using wstrmatch = std::match_results<wstr::iterator>;
}


namespace std {

    template<std::integral CharT>
    std::basic_istream<CharT>& getline(std::basic_istream<CharT>& is, Auxil::BasicStr<CharT>& s, CharT delim) {
        using traits_t = typename Auxil::BasicStr<CharT>::traits_type;
        s.clear();

        typename traits_t::int_type c = is.rdbuf()->sgetc();

        bool extracted = false;
        while (!traits_t::eq_int_type(c, traits_t::eof()))
        {
            is.rdbuf()->sbumpc();
            CharT ch = traits_t::to_char_type(c);

            if (ch == delim) {
                return is;
            }

            s.push_back(ch);
            extracted = true;
            c = is.rdbuf()->sgetc();
        }

        if (!extracted) {
            is.setstate(std::ios_base::failbit);
        } else {
            is.setstate(std::ios_base::eofbit);
        }
        return is;
    }

    template<std::integral CharT>
    std::basic_istream<CharT>& getline(std::basic_istream<CharT>& is, Auxil::BasicStr<CharT>& s) {
        return getline(is, s, static_cast<CharT>('\n'));
    }


    template<std::integral CharT>
    struct formatter<Auxil::BasicStr<CharT>, CharT> : formatter<const CharT*, CharT> {

        template<typename ParseContext>
        auto parse(ParseContext& ctx) {
            return formatter<const CharT*, CharT>::parse(ctx);
        }

        template<typename FormatContext>
        auto format(const Auxil::BasicStr<CharT>& s, FormatContext& ctx) const {
            return formatter<const CharT*, CharT>::format(s.c_str(), ctx);
        }
    };


    template<std::integral CharT>
    struct hash<Auxil::BasicStr<CharT>> {
        Auxil::usize operator()(const Auxil::BasicStr<CharT>& s) const noexcept {
            Auxil::usize h = 0;
            for (size_t i = 0; i < s.length(); ++i) {
                h = h * 31 + static_cast<Auxil::usize>(s[i]);
            }
            return h;
        }
    };
}

#endif