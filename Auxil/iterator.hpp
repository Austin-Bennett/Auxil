#ifndef ITERATOR_HPP
#define ITERATOR_HPP
#include <type_traits>
#include "misc.hpp"

namespace Auxil {

    using namespace Primitives;

    template<typename T>
    concept ForwardIterator =
        // Require nested types
        requires {
        typename T::value_type;
        typename T::difference_type;
        typename T::pointer;
        typename T::reference;
        typename T::iterator_category;
        } &&
        // Require the usual iterator operations
        std::input_or_output_iterator<T> &&
        std::default_initializable<T> &&
        std::copyable<T> &&
        std::equality_comparable<T> &&
        //require some type stuff
        std::is_signed_v<typename T::difference_type> &&

        requires(T iter) {
        { *iter } -> std::same_as<typename T::reference>;
        { iter.operator ->() } -> std::same_as<typename T::pointer>;
        { ++iter } -> std::same_as<T&>;
        { iter++ } -> std::convertible_to<T>;
    };

    template<typename T>
    concept BidirectionalIterator =
        // Require nested types
        requires {
        typename T::value_type;
        typename T::difference_type;
        typename T::pointer;
        typename T::reference;
        typename T::iterator_category;
        } &&
        // Require the usual iterator operations
        std::input_or_output_iterator<T> &&
        std::default_initializable<T> &&
        std::copyable<T> &&
        std::equality_comparable<T> &&

        std::is_signed_v<typename T::difference_type> &&

        requires(T iter) {
        { *iter } -> std::same_as<typename T::reference>;
        { iter.operator ->() } -> std::same_as<typename T::pointer>;
        { ++iter } -> std::same_as<T&>;
        { iter++ } -> std::convertible_to<T>;
        { --iter } -> std::same_as<T&>;
        { iter-- } -> std::convertible_to<T>;
        };

    template<typename T>
    concept Iterable = requires(T iter) {
        { iter.begin() } -> ForwardIterator;
        { iter.end() } -> ForwardIterator;
        { iter.size() } -> std::convertible_to<std::size_t>;
    };

    template<typename T>
    concept ReverseIterable = requires(T iter) {
        { iter.rbegin() };
        { iter.rend() };
    };

#include <concepts>
#include <type_traits>




    template<Iterable T>
    struct iterator_from_iterable {
        using type = decltype(std::declval<T>().begin());
    };

    template<Iterable T>
    using iterator_from_iterable_t = typename iterator_from_iterable<T>::type;

    template<ReverseIterable T>
    struct reverse_iterator_from_iterable {
        using type = decltype(std::declval<T>().rbegin());
    };


    template<ReverseIterable T>
    using reverse_iterator_from_iterable_t = typename reverse_iterator_from_iterable<T>::type;

    template<ForwardIterator T>
    using iterator_value_t = typename T::value_type;

    template<ForwardIterator T>
    using iterator_difference_t = typename T::difference_type;

    template<ForwardIterator T>
    using iterator_reference_t = typename T::reference_type;

    template<ForwardIterator T>
    using iterator_pointer_t = typename T::pointer_type;


    template<Iterable T>
    struct iterable_value {
        using type = iterator_value_t<iterator_from_iterable_t<T>>;
    };

    template<Iterable T>
    using iterable_value_t = typename iterable_value<T>::type;

    template<typename It>
    class GenericIterable {
    protected:
        It _begin;
        It _end;
    public:
        GenericIterable(It beg, It end) : _begin(beg), _end(end) {}

        template<Iterable T>
        explicit GenericIterable(T iterable) : _begin(iterable.begin()), _end(iterable.end()) {

        }

        It begin() const {
            return _begin;
        }

        It end() const {
            return _end;
        }

        usize size() const {
            return _end-_begin;
        }
    };

    template<ReverseIterable T>
    GenericIterable<reverse_iterator_from_iterable_t<T>> reverse(T& iterable) {
        return GenericIterable(iterable.rbegin(), iterable.rend());
    }

    template<ForwardIterator It1, ForwardIterator It2>
    class ZipIterator {

        It1 iter1;
        It2 iter2;
    public:
        using iterator_category = std::forward_iterator_tag; // Assuming both It1 and It2 are forward iterators
        using value_type = std::pair<
            typename std::iterator_traits<It1>::value_type,
            typename std::iterator_traits<It2>::value_type>;
        using difference_type = std::pair<iterator_difference_t<It1>, iterator_difference_t<It2>>;
        using reference = std::pair<typename std::iterator_traits<It1>::reference, typename std::iterator_traits<It2>::reference>;
        using pointer = std::pair<typename std::iterator_traits<It1>::pointer, typename std::iterator_traits<It2>::pointer>;

        ZipIterator() = default;

        ZipIterator(It1 it1, It2 it2)
        : iter1(it1), iter2(it2) {}

        reference operator*() const {
            return reference{*iter1, *iter2};
        }

        // Pre-increment
        ZipIterator& operator++() {
            ++iter1;
            ++iter2;
            return *this;
        }

        // Post-increment
        ZipIterator operator++(int) {
            ZipIterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const ZipIterator& other) const {
            return iter1 == other.iter1 || iter2 == other.iter2;
        }

        bool operator!=(const ZipIterator& other) const {
            return !(*this == other);
        }
    };

    template<Iterable I1, Iterable I2>
    GenericIterable<ZipIterator<iterator_from_iterable_t<I1>, iterator_from_iterable_t<I2>>>
    zip(I1& i1, I2& i2) {
        using Iter = ZipIterator<iterator_from_iterable_t<I1>, iterator_from_iterable_t<I2>>;

        return {Iter(i1.begin(), i2.begin()), Iter(i1.end(), i2.end())};
    }


    template<Iterable I1, Iterable I2>
    GenericIterable<ZipIterator<iterator_from_iterable_t<I1>, iterator_from_iterable_t<I2>>>
    zip_copy(I1 i1, I2 i2) {
        using Iter = ZipIterator<iterator_from_iterable_t<I1>, iterator_from_iterable_t<I2>>;

        return {Iter(i1.begin(), i2.begin()), Iter(i1.end(), i2.end())};
    }


    // template<typename T>
    // concept ForwardIterator =
    //     // Require nested types
    //     requires {
    //     typename T::value_type;
    //     typename T::difference_type;
    //     typename T::pointer;
    //     typename T::reference;
    //     typename T::iterator_category;
    //     } &&
    //     // Require the usual iterator operations
    //     std::input_or_output_iterator<T> &&
    //     std::default_initializable<T> &&
    //     std::copyable<T> &&
    //     std::equality_comparable<T> &&
    //
    //     requires(T iter) {
    //     { *iter } -> std::same_as<typename T::reference>;
    //     { ++iter } -> std::same_as<T&>;
    //     { iter++ } -> std::convertible_to<T>;
    // };

    template<typename T>
    class PointerIterator {
        T* ptr;

    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::bidirectional_iterator_tag;

        PointerIterator() : ptr(nullptr) {}

        PointerIterator(T* p) : ptr(p) {}

        reference operator *() const {
            return *ptr;
        }

        pointer operator->() const {
            return ptr;
        }

        PointerIterator& operator++() {
            ++ptr;
            return *this;
        }

        PointerIterator operator++(int) {
            auto res = *this;

            ++ptr;

            return res;
        }

        PointerIterator& operator --() {
            --ptr;

            return *this;
        }

        PointerIterator operator--(int) {
            auto res = *this;

            --ptr;

            return *this;
        }

        PointerIterator operator+(difference_type amt) {
            auto res = *this;

            res.ptr += amt;

            return res;
        }

        PointerIterator operator-(difference_type amt) {
            auto res = *this;

            res.ptr -= amt;

            return res;
        }

        PointerIterator& operator+=(difference_type amt) {
            ptr += amt;

            return *this;
        }

        PointerIterator& operator-=(difference_type amt) {
            ptr -= amt;

            return *this;
        }

        bool operator==(const PointerIterator &other) const {
            return other.ptr == ptr;
        }

        bool operator!=(const PointerIterator &other) const {
            return other.ptr != ptr;
        }
    };

    template<typename T>
    class ReversePointerIterator {
        T* ptr;

    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::bidirectional_iterator_tag;

        ReversePointerIterator() : ptr(nullptr) {}

        ReversePointerIterator(T* p) : ptr(p) {}

        reference operator *() const {
            return *ptr;
        }

        pointer operator->() const {
            return ptr;
        }

        ReversePointerIterator& operator++() {
            --ptr;
            return *this;
        }

        ReversePointerIterator operator++(int) {
            auto res = *this;

            --ptr;

            return res;
        }

        ReversePointerIterator& operator --() {
            ++ptr;

            return *this;
        }

        ReversePointerIterator operator--(int) {
            auto res = *this;

            ++ptr;

            return *this;
        }

        ReversePointerIterator operator+(difference_type amt) {
            auto res = *this;

            res.ptr -= amt;

            return res;
        }

        ReversePointerIterator operator-(difference_type amt) {
            auto res = *this;

            res.ptr += amt;

            return res;
        }

        ReversePointerIterator& operator+=(difference_type amt) {
            ptr -= amt;

            return *this;
        }

        ReversePointerIterator& operator-=(difference_type amt) {
            ptr += amt;

            return *this;
        }

        bool operator==(const ReversePointerIterator &other) const {
            return other.ptr == ptr;
        }

        bool operator!=(const ReversePointerIterator &other) const {
            return other.ptr != ptr;
        }
    };

    template<typename T>
    GenericIterable<PointerIterator<T>> iterate_pointer(T* begin, T* end) {
        return GenericIterable<PointerIterator<T>>(PointerIterator<T>(begin), PointerIterator<T>(end));
    }

}

#endif