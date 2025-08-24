#ifndef CONTAINERS_HPP
#define CONTAINERS_HPP
#include <vector>
#include <algorithm>
#include "iterator.hpp"
#include "exception.hpp"
#include "math.hpp"

namespace Auxil {
    //A fixed-size array, but can be allocated at runtime
    template<std::semiregular T>
    class Array {
    protected:
        T* _arr = nullptr;
        usize _size{0};
        union Flags {
            u8 flags{0};
            struct {
                bool is_pointer_wrapper: 1;
            };
        } flags{};

        void destruct() {
            if (!flags.is_pointer_wrapper) delete[] _arr;
            _size = 0;
            _arr = nullptr;
        }

        void initialize() {
            for (usize i = 0; i < _size; i++) {
                new (_arr+i) T();
            }
        }

    public:
        ~Array() {
            destruct();
        }

        Array() = default;

        explicit Array(const usize size) {
            _arr = new T[size];
            _size = size;
            initialize();
        }

        Array(const std::initializer_list<T>& init) {
            _arr = new T[init.size()];
            _size = init.size();
            auto it = init.begin();

            for (usize i = 0; i < _size; i++) {
                _arr[i] = *(it++);
            }
        }

        Array(T* ptr, usize size) {
            flags.is_pointer_wrapper = true;
            _arr = ptr;
            _size = size;
        }

        template<Iterable I, typename = std::enable_if_t<std::is_convertible_v<iterable_value_t<I>, T>>>
        explicit Array(const I& iterable) {
            _size = static_cast<usize>(iterable.size());
            _arr = new T[_size];
            for (auto[member, init_list]: Auxil::zip_copy(
                iterate_pointer(_arr, _arr+_size),
                iterable
            )) {
                member = init_list;
            }
        }

        Array(const Array& arr) {
            _size = arr.size();
            _arr = new T[_size];
            for (usize i = 0; i < _size; i++) {
                std::construct_at(_arr+i, arr[i]);
            }
        }

        Array(Array&& arr)  noexcept {
            _size = arr._size;
            _arr = arr._arr;
            arr._arr = nullptr;
            arr.destruct();
        }

        Array& operator=(const usize size) {
            destruct();
            _arr = new T[size];
            _size = size;

            return *this;
        }

        Array& operator=(const std::initializer_list<T>& init) {
            destruct();
            _arr = new T[init.size()];
            _size = init.size();

            for (auto[member, init_list]:
                Auxil::zip_copy(
                    iterate_pointer(_arr, _arr+_size),
                    iterate_pointer(init.begin(), init.end()))) {
                member = init_list;
                    }

            return *this;
        }

        Array& operator=(const Array& arr) {
            if (&arr == this) return *this;
            destruct();

            _size = arr.size();
            _arr = new T[_size];
            for (usize i = 0; i < _size; i++) {
                std::construct_at(_arr+i, arr[i]);
            }

            return *this;
        }

        template<Iterable I, typename = std::enable_if_t<std::is_convertible_v<iterable_value_t<I>, T>>>
        Array& operator=(const I& iterable) {
            destruct();
            _size = static_cast<usize>(iterable.size());
            _arr = new T[_size];
            for (auto[member, init_list]: Auxil::zip_copy(
                iterate_pointer(_arr, _arr+_size),
                iterable
            )) {
                member = init_list;
            }

            return *this;
        }

        Array& operator=(Array&& arr) noexcept {
            if (&arr == this) return *this;

            destruct();
            _size = arr._size;
            _arr = arr._arr;
            arr._arr = nullptr;
            arr.destruct();

            return *this;
        }

        [[nodiscard]] bool is_pointer_wrapper() {
            return flags.is_pointer_wrapper;
        }

        [[nodiscard]] bool empty() const {
            return _size == 0;
        }

        [[nodiscard]] usize size() const {
            return _size;
        }

        PointerIterator<T> begin() const {
            return _arr;
        }

        PointerIterator<T> end() const {
            return _arr+_size;
        }

        ReversePointerIterator<T> rbegin() const {
            return _arr+size()-1;
        }

        ReversePointerIterator<T> rend() const {
            return _arr-1;
        }


        FORCE_INLINE T& front() const {
            return (*_arr);
        }

        FORCE_INLINE T& back() const {
            return *(_arr+_size-1);
        }

        FORCE_INLINE T& at(usize ind) const {
            if (ind >= _size) throw Auxil::Exception("Cannot access element at index {}", ind);
            return _arr[ind];
        }

        FORCE_INLINE T& operator[](usize ind) const {
            if (ind >= _size) throw Auxil::Exception("Cannot access element at index {}", ind);
            return _arr[ind];
        }

        template<typename... Args>
        FORCE_INLINE T& emplace_at(usize ind, Args&&... constructor) const {
            std::destroy_at(_arr + ind);
            T& res = *(new (_arr + ind) T(std::forward<Args>(constructor)...));
            return res;
        }

        FORCE_INLINE T* data() const {
            return _arr;
        }


    };

    template<typename U, typename = std::enable_if_t<OstreamFormattable<U>>>
    std::ostream& operator <<(std::ostream& os, const Array<U>& arr) {
        os << '[';

        for (usize i = 0; i < arr._size; i++) {
            os  << arr[i];
            if (i != arr._size-1) {
                os << ", ";
            }
        }

        os << ']';

        return os;
    }



    template<typename T, typename = std::enable_if_t<std::semiregular<T>>>
    class Grid {
        T* matrix{nullptr};
        usize _columns{0}, _rows{0};


        void initialize(const usize rows, const usize columns, bool default_initialize = false) {
            matrix = static_cast<T*>(::operator new[](sizeof(T)*columns*rows));
            this->_columns = columns;
            this->_rows = rows;

            if (default_initialize) {
                for (usize i = 0; i < columns*rows; i++) {
                    new (matrix+i) T();
                }
            }
        }

        void destruct() {
            for (usize i = 0; i < _columns*_rows; i++) {
                matrix[i].~T();
            }

            ::operator delete[](matrix);
            matrix = nullptr;
            _columns = 0;
            _rows = 0;
        }


        FORCE_INLINE static Grid dot(const Grid& left, const Grid& right) {

            Grid result = make(left._rows, right._columns);

            for (usize i = 0; i < left._rows; i++) {
                for (usize j = 0; j < right._columns; j++) {
                    T sum = 0;
                    for (usize k = 0; k < left._columns; k++) {
                        sum += left[i][k] * right[k][j];
                    }
                    result[i][j] = sum;
                }
            }

            return result;
        }

    public:

        ~Grid() {
            destruct();
        }

        Grid() = default;

        [[nodiscard]] static Grid make(const usize rows, const usize columns) {
            Grid res;
            res.initialize(rows, columns, true);

            return res;
        }


        Grid(std::initializer_list<std::initializer_list<T>> init_list) {
            auto it = init_list.begin();
            initialize(init_list.size(), it->size(), true);
            usize i = 0;
            for (; it != init_list.end(); ++it) {
                for (usize j = 0; j < it->size() && j < _columns; j++) {
                    std::construct_at(&matrix[i++], *((*it).begin()+j));
                }
            }
        }

        Grid(const Grid& grid) noexcept {
            if (grid.rows == 0) return;
            initialize(grid._rows, grid._columns);

            for (usize i = 0; i < grid.size(); i++) {
                std::construct_at(&matrix[i], grid.matrix[i]);
            }
        }

        Grid(Grid&& grid) noexcept {
            if (grid._rows == 0) return;

            matrix = grid.matrix;
            _columns = grid._columns;
            _rows = grid._rows;
            grid.matrix = nullptr;
            grid.destruct();
        }

        Grid& operator=(const Grid& grid) noexcept {
            if (grid.rows == 0) return *this;
            if (&grid == this) return *this;

            destruct();

            initialize(grid._rows, grid._columns);

            for (usize i = 0; i < grid.size(); i++) {
                std::construct_at(&matrix[i], grid.matrix[i]);
            }

            return *this;
        }

        Grid& operator=(Grid&& grid) noexcept {
            if (grid._rows == 0) return *this;
            if (&grid == this) return *this;

            destruct();

            matrix = grid.matrix;
            _rows = grid._rows;
            _columns = grid._columns;
            grid.matrix = nullptr;
            grid.destruct();

            return *this;
        }

        template<typename... Args>
        FORCE_INLINE T& emplace_at(usize row, usize column, Args&&... constructor) {
            if (row >= _rows || column >= _columns) {
                throw Exception("Cannot create element at ({}, {}) in Grid with dimensions {}x{}", column, row, _rows, _columns);
            }
            std::destroy_at((matrix+row*_columns)+column);
            auto& elem = *std::construct_at((matrix+row*_columns), std::forward<Args>(constructor)...);
            return elem;
        }

        [[nodiscard]] FORCE_INLINE usize width() const {
            return _columns;
        }

        [[nodiscard]] FORCE_INLINE  usize height() const {
            return _rows;
        }

        [[nodiscard]] FORCE_INLINE usize area() const {
            return _rows*_columns;
        }

        [[nodiscard]] FORCE_INLINE usize columns() const {
            return _columns;
        }

        [[nodiscard]] FORCE_INLINE  usize rows() const {
            return _rows;
        }

        [[nodiscard]] FORCE_INLINE  usize size() const {
            return _rows*_columns;
        }

        FORCE_INLINE Array<T> at(const usize row_ind) const {
            if (row_ind >= _rows) throw Exception("{} out of range of Grid with {} rows", row_ind, _rows);
            return Array<T>(matrix+(row_ind*_columns), _columns);
        }

        FORCE_INLINE T& at_flat(const usize ind) const {
            if (ind >= _rows*_columns) throw Exception("{} out of range of Grid with {} elements", ind, _rows*_columns);
            return matrix[ind];
        }

        FORCE_INLINE Array<T> operator[](const usize row_ind) const {
            if (row_ind >= _rows) throw Exception("{} out of range of Grid with {} rows", row_ind, _rows);
            return Array<T>(matrix+(row_ind*_columns), _columns);
        }

        FORCE_INLINE Array<T> front() const {
            if (_rows == 0) throw Exception("Cannot access front of empty grid");
            return Array<T>(matrix, _columns);
        }

        FORCE_INLINE Array<T> back() const {
            if (_rows == 0) throw Exception("Cannot access back of empty grid");
            return Array<T>(matrix+((_rows-1)*_columns), _columns);
        }

        FORCE_INLINE T& first() const {
            if (_rows * columns == 0) throw Exception("Cannot access first element of a {}x{} grid", _rows, _columns);
            return matrix[0];
        }

        FORCE_INLINE T& last() const {
            if (_rows * columns == 0) throw Exception("Cannot access last element of a {}x{} grid", _rows, _columns);
            return matrix[_rows*_columns-1];
        }

        FORCE_INLINE T* data() const {
            return matrix;
        }

        PointerIterator<T> begin() const {
            return matrix;
        }

        PointerIterator<T> end() const {
            return matrix+(_rows*_columns);
        }

        ReversePointerIterator<T> rbegin() const {
            return matrix+((_rows*_columns)-1);
        }

        ReversePointerIterator<T> rend() const {
            return matrix-1;
        }

        [[nodiscard]] FORCE_INLINE bool empty() const {
            return _rows*_columns == 0;
        }

        Grid& reset() {
            for (usize i = 0; i < _rows*_columns; i++) {
                matrix[i] = T();
            }

            return *this;
        }


        Grid operator+(const Grid& other) const {
            if (_rows != other.rows || _columns != other.columns) {
                throw Exception("Cannot add grids of different sizes.\n"
                                "Note, right matrix had dimensions {}x{}, expected {}x{}", other.rows, other.columns,
                                _rows, _columns);
            }

            Grid result = make(_rows, _columns);
            for (usize i = 0; i < size(); i++) {
                std::construct_at(&result.matrix[i], matrix[i] + other.matrix[i]);
            }

            return result;
        }


        Grid operator-(const Grid& other) const {
            if (_rows != other.rows || _columns != other.columns) {
                throw Exception("Cannot subtract grids of different sizes.\n"
                                "Note, right matrix had dimensions {}x{}, expected {}x{}", other.rows, other.columns,
                                _rows, _columns);
            }

            Grid result = make(_rows, _columns);
            for (usize i = 0; i < size(); i++) {
                std::construct_at(&result.matrix[i], matrix[i] - other.matrix[i]);
            }

            return result;
        }


        Grid& operator+=(const Grid& other) {
            if (_rows != other.rows || _columns != other.columns) {
                throw Exception("Cannot add grids of different sizes.\n"
                                "Note, right matrix had dimensions {}x{}, expected {}x{}", other.rows, other.columns,
                                _rows, _columns);
            }

            for (usize i = 0; i < size(); i++) {
                matrix[i] += other.matrix[i];
            }

            return this;
        }


        Grid& operator-=(const Grid& other) {
            if (_rows != other.rows || _columns != other.columns) {
                throw Exception("Cannot subtract grids of different sizes.\n"
                                "Note, right matrix had dimensions {}x{}, expected {}x{}", other.rows, other.columns,
                                _rows, _columns);
            }

            for (usize i = 0; i < size(); i++) {
                matrix[i] -= other.matrix[i];
            }

            return this;
        }




        FORCE_INLINE Grid dot(const Grid& right) const {
            if (_columns != right._rows) {
                if (right._columns == _rows) return dot(right, *this);
                throw Exception("Cannot compute the dot product of grids where the columns of one"
                                " does not equal the number of rows of the other\n"
                                "Left: {}x{}, Right: {}x{}", _rows, _columns, right._rows, right._columns);
            }

            return dot(*this, right);
        }



        Grid operator*(const Grid& other) {
            return dot(other);
        }


        //this one does a hadamard multiplication
        Grid& operator*=(const Grid& other) {
            if (_rows != other._rows || _columns != other._columns) {
                throw Exception("Cannot perform a hadamard operation (multiplication) on grids of different sizes.\n"
                                "Note, right matrix had dimensions {}x{}, expected {}x{}", other._rows, other._columns,
                                _rows, _columns);
            }

            for (usize i = 0; i < size(); i++) {
                matrix[i] *= other.matrix[i];
            }

            return this;
        }


        //hadamard multiplication
        Grid multiply(const Grid& other) {
            if (_rows != other.rows || _columns != other._columns) {
                throw Exception("Cannot perform a hadamard operation (multiplication) on grids of different sizes.\n"
                                "Note, right matrix had dimensions {}x{}, expected {}x{}", other._rows, other._columns,
                                _rows, _columns);
            }

            Grid res = make(_rows, _columns);
            res *= other;

            return res;
        }

        template<Arithmetic U>
        Grid operator /(U scalar) {
            Grid res = make(_rows, _columns);
            for (usize i = 0; i < res.size(); i++) {
                res.matrix[i] = matrix[i] / scalar;
            }

            return res;
        }

        template<Arithmetic U>
        Grid& operator /=(U scalar) {

            for (usize i = 0; i < size(); i++) {
                matrix[i] /= scalar;
            }

            return *this;
        }

        //TODO: add hadamard division
    };

    template<typename U, typename = std::enable_if_t<OstreamFormattable<U>>>
        std::ostream& operator<<(std::ostream& os, const Grid<U>& self) {
        os << '[';
        for (usize r = 0; r < self._rows; r++) {
            os << '[';
            for (usize c = 0; c < self._columns; c++) {
                os << self.matrix[r*self._columns + c];
                if (c != self._columns-1) os << ", ";
            }
            os << ']';
            if (r != self._rows-1) os << ", ";
        }
        os << ']';

        return os;
    }


    /*
     * A container designed to be a sort of
     * Army swiss knife of linear containers that can grow, combining the allocation efficiency of a std::vector
     * while also having the insertion, and removal speed of a linked list, random element access is still slower than
     * a vector, but accessing the front and back elements is O(1) still with some simple data management
     *
     * CONSIDER: sort elements in the internal array by their relative position
     */
    template<typename T, typename = std::enable_if_t<std::semiregular<T>>>
    class LinkedList {
    public:
        static constexpr usize npos = std::numeric_limits<usize>::max();
        struct Node {
            //if this node is valid
            bool valid{false};
            T value{};
            //indices into whatever is next or previous
            usize prev{npos};
            usize next{npos};

            void reset() {
                valid = false;
                value = T();
                prev = npos;
                next = npos;
            }
        };
    private:

        //makes it easier to manage memory allocation
        //to make life easy, the start of the list will always be the first element, and likewise the last element
        //will be right after it
        //also, to make appending, popping, and other good stuff just as fast as a linked list, all empty elements
        //will be moved to the back of the array by swapping their position with the current last element
        Array<Node> nodes{};
        usize length{0};
        mutable usize cur_index{npos};

        void swap(usize i1, usize i2) {

            if (cur_index == i1) cur_index = i2;
            else if (cur_index == i2) cur_index = i1;

            Node& one = nodes[i1];
            Node& two = nodes[i2];

            bool change2_prev{true};
            bool change2_next{true};


            if (one.next != npos) {
                if (one.next == i2) {
                    nodes[one.next].prev = i2;
                    one.next = i1;
                    change2_prev = false;
                } else {
                    nodes[one.next].prev = i2;
                }
            }
            if (one.prev != npos) {
                if (one.prev == i2) {
                    nodes[one.prev].next = i2;
                    one.prev = i1;
                    change2_next = false;
                } else {
                    nodes[one.prev].next = i2;
                }
            }

            if (two.next != npos && change2_next) nodes[two.next].prev = i1;
            if (two.prev != npos && change2_prev) nodes[two.prev].next = i1;

            std::swap(one, two);
        }

        void destruct() {
            nodes.~Array();
            length = 0;
            cur_index = npos;
        }

    public:

        ~LinkedList() {
            destruct();
        }

        LinkedList() = default;

        LinkedList(const std::initializer_list<T>& elements) {
            reserve(elements.size());
            for (auto& element: elements) {
                push_back(element);
            }
        }

        LinkedList(const LinkedList& other) {
            if (other.empty()) return;
            reserve(other.length);

            usize node = 0;
            while (node != npos) {
                push_back(other.nodes[node].value);
                node = other.nodes[node].next;
            }
        }

        LinkedList(LinkedList&& other)  noexcept {
            nodes = std::move(other.nodes);
            length = other.length;
            cur_index = other.cur_index;
            other.destruct();
        }

        LinkedList& operator=(const LinkedList& other) {
            if (&other == this) return *this;
            destruct();
            if (other.empty()) return *this;
            reserve(other.length);

            usize node = 0;
            while (node != npos) {
                push_back(other.nodes[node].value);
                node = other.nodes[node].next;
            }

            return *this;
        }

        LinkedList& operator=(LinkedList&& other) noexcept {
            if (&other == this) return *this;
            nodes = std::move(other.nodes);
            length = other.length;
            cur_index = other.cur_index;
            other.destruct();

            return *this;
        }



        //reserves allocated memory
        LinkedList& reserve(usize new_size) {
            Array new_allocation = Array<Node>(new_size);

            auto rng = nodes | std::views::take(new_size);
            std::ranges::move(rng, new_allocation.begin());
            length = std::min(new_size, length);

            nodes = std::move(new_allocation);

            return *this;
        }

        //reserves memory, but appends elements to the list
        LinkedList& resize(usize new_size) {
            if (new_size <= length) {
                reserve(new_size);
                return *this;
            }
            new_size -= length;
            reserve(new_size+length);

            while (new_size-- > 0) {
                push_back(T());
            }

            return *this;
        }

        //--------- Adding values -----------

        //appends a value to the end of the list
        T& push_back(const T& value) {
            if (length == nodes.size()) {
                //ensures we get at least enough space for the new element
                reserve(nodes.size()*2+1);
            }
            //if the array is empty, there is no 'back'
            //if the array has just 1 element, then the back is also the front, so the zero'th element,
            //if the array has 2 or more elements, then the back should be the first element so long as
            //nothing has gone wrong
            usize back_ind = length > 0 /*If empty*/ ? (length >= 2 /*If it has 2 or more elements*/ ? 1:0) : npos;
            Node& n = nodes.emplace_at(length++, true, value, back_ind, npos);

            //set it to 1 because we swap its current position with 1 later, if this is the only element currently added
            //then the npos check will stop anything from happening,
            if (back_ind != npos) nodes[back_ind].next = length-1;

            //this is to make sure we keep the last element in the second index
            //this also will fix the fact that the back index which we pointed to previously has now changed and all
            //references should stay consistent
            if (length > 2) swap(1, length-1);

            if (cur_index == npos) cur_index = 0;

            //return the new value
            return n.value;
        }

        //appends a value to the start of the list
        T& push_front(const T& value) {
            if (length == nodes.size()) {
                //ensures we get at least enough space for the new element
                reserve(nodes.size()*2+1);
            }
            //if the array is empty, there is no 'back'
            //if the array has 1 or more elements, then the front is index 0
            //nothing has gone wrong
            usize start_ind = length > 0 /*If not empty*/ ? 0 : npos;
            Node& n = nodes.emplace_at(length++, true, value, npos, start_ind);

            //set it to 1 because we swap its current position with 1 later, if this is the only element currently added
            //then the npos check will stop anything from happening,
            if (start_ind != npos) nodes[start_ind].prev = length-1;

            //this is to make sure we keep the last element in the second index
            //this also will fix the fact that the back index which we pointed to previously has now changed and all
            //references should stay consistent
            if (length > 1) swap(0, length-1);

            if (cur_index == npos) cur_index = 0;

            //return the new value
            return n.value;
        }

        //moves a value to the end of the list
        T& push_back(const T&& value) {
            if (length == nodes.size()) {
                //ensures we get at least enough space for the new element
                reserve(nodes.size()*2+1);
            }
            //if the array is empty, there is no 'back'
            //if the array has just 1 element, then the back is also the front, so the zeroth element,
            //if the array has 2 or more elements, then the back should be the first element so long as
            //nothing has gone wrong
            usize back_ind = length > 0 /*If empty*/ ? (length >= 2 /*If it has 2 or more elements*/ ? 1:0) : npos;
            Node& n = nodes.emplace_at(length++, true, value, back_ind, npos);

            //set it to length-1 because we swap its current position with 1 later, if this is the only element currently added
            //then the npos check will stop anything from happening,
            if (back_ind != npos) nodes[back_ind].next = length-1;

            //this is to make sure we keep the last element in the second index
            //this also will fix the fact that the back index which we pointed to previously has now changed and all
            //references should stay consistent
            if (length > 2)
                swap(1, length-1);

            if (cur_index == npos) cur_index = 0;

            //return the new value
            return n.value;
        }

        //moves a value to the start of the list
        T& push_front(const T&& value) {
            if (length == nodes.size()) {
                //ensures we get at least enough space for the new element
                reserve(nodes.size()*2+1);
            }
            //if the array is empty, there is no 'back'
            //if the array has 1 or more elements, then the front is index 0
            //nothing has gone wrong
            usize start_ind = length > 0 /*If not empty*/ ? 0 : npos;
            Node& n = nodes.emplace_at(length++, true, value, npos, start_ind);

            //if this is the only element currently added then the npos check will stop anything from happening
            if (start_ind != npos) nodes[start_ind].prev = length-1;

            //this is to make sure we keep the last element in the second index
            //this also will fix the fact that the back index which we pointed to previously has now changed and all
            //references should stay consistent
            if (length > 1) swap(0, length-1);

            if (cur_index == npos) cur_index = 0;

            //return the new value
            return n.value;
        }

        //pushes a value in front of the current node in the list
        T& push_ahead(const T& value) {
            if (length == nodes.size()) {
                reserve(nodes.size()*2)+1;
            }
            Node* cur = nullptr;
            if (cur_index != npos) cur = &nodes[cur_index];
            Node& n = nodes.emplace_at(length++, true, value, cur_index, cur ? cur->next:npos);
            if (cur) {
                if (cur->next != npos) nodes[cur->next].prev = length-1;
                cur->next = length-1;
            }
            //if this is the new end
            if (n.next == npos && length > 2) {
                swap(length-1, 1);
            }

            if (cur_index == npos) cur_index = 0;

            return n.value;
        }

        //pushes a value in behind the current node in the list
        T& push_behind(const T& value) {
            if (length == nodes.size()) {
                reserve(nodes.size()*2)+1;
            }
            Node* cur = nullptr;
            if (cur_index != npos) cur = &nodes[cur_index];
            Node& n = nodes.emplace_at(length++, true, value, cur ? cur->prev:npos, cur_index);
            if (cur) {
                if (cur->prev != npos) nodes[cur->prev].next = length-1;
                cur->prev = length-1;
            }
            //if this is the new beginning
            if (n.prev == npos && length > 1) {
                swap(length-1, 0);
            }

            if (cur_index == npos) cur_index = 0;

            return n.value;
        }

        //moves a value in front of the current node in the list
        T& push_ahead(const T&& value) {
            if (length == nodes.size()) {
                reserve(nodes.size()*2)+1;
            }
            Node* cur = nullptr;
            if (cur_index != npos) cur = &nodes[cur_index];
            Node& n = nodes.emplace_at(length++, true, value, cur_index, cur ? cur->next:npos);
            if (cur) {
                if (cur->next != npos) nodes[cur->next].prev = length-1;
                cur->next = length-1;
            }
            //if this is the new end
            if (n.next == npos && length > 2) {
                swap(length-1, 1);
            }

            if (cur_index == npos) cur_index = 0;

            return n.value;
        }

        //moves a value in behind the current node in the list
        T& push_behind(const T&& value) {
            if (length == nodes.size()) {
                reserve(nodes.size()*2)+1;
            }
            Node* cur = nullptr;
            if (cur_index != npos) cur = &nodes[cur_index];
            Node& n = nodes.emplace_at(length++, true, value, cur ? cur->prev:npos, cur_index);
            if (cur) {
                if (cur->prev != npos) nodes[cur->prev].next = length-1;
                cur->prev = length-1;
            }
            //if this is the new beginning
            if (n.prev == npos && length > 1) {
                swap(length-1, 0);
            }

            if (cur_index == npos) cur_index = 0;

            return n.value;
        }

        //--------- Removing Items -----------

        LinkedList& pop_ahead() {
            if (cur_index == npos) return *this;
            Node* cur = &nodes[cur_index];
            Node* next = nullptr;
            if (cur->next != npos) next = &nodes[cur->next];

            if (next) {
                //swap references
                usize nex_id = cur->next;
                cur->next = next->next;
                if (cur->next != npos) nodes[cur->next].prev = cur_index;

                //check if next was the previous back, and if so, make the cur index the back
                if (next->next == npos) {
                    usize back_ind = length > 1 ? 1 : 0;
                    swap(cur_index, back_ind);
                }

                //makes sure we don't accidentally use this node
                next->reset();
                //swap with the last element
                swap(nex_id, length-1);

                length--;
            }

            return *this;
        }

        LinkedList& pop_behind() {
            if (cur_index == npos) return *this;
            Node* cur = &nodes[cur_index];
            Node* prev = nullptr;
            if (cur->prev != npos) prev = &nodes[cur->prev];

            if (prev) {
                //swap references
                usize prev_id = cur->prev;
                cur->prev = prev->prev;
                if (cur->prev != npos) nodes[cur->prev].next = cur_index;

                //check if next was the previous start, and if so, make the cur index the back
                if (prev->prev == npos) {
                    swap(cur_index, 0);
                }

                //makes sure we don't accidentally use this node
                prev->reset();
                //swap with the last element
                swap(prev_id, length-1);

                length--;
            }

            return *this;
        }

        //pops this node and attempts to advance to the next node
        LinkedList& pop_advance() {
            if (cur_index == npos) return *this;
            Node* cur = &nodes[cur_index];
            Node* next = nullptr;
            Node* prev = nullptr;
            if (cur->next != npos) next = &nodes[cur->next];
            if (cur->prev != npos) prev = &nodes[cur->prev];

            if (next) {
                //first check if its the front
                if (cur->prev == npos) {
                    swap(cur->next, 0);
                }

                //replace references
                next->prev = cur->prev;
                if (prev) prev->next = cur->next;

                usize cind = cur_index;
                cur_index = cur->next;

                cur->reset();

                //move to the back
                swap(cind, length-1);
            } else if (prev) {
                //cur must be the last node
                swap(cur->prev, length > 1 ? 1 : 0);


                //we dont have to update next because its null
                prev->next = npos;

                usize cind = cur_index;
                //only difference if we have to move back
                cur_index = cur->prev;

                cur->reset();

                //move to the back
                swap(cind, length-1);
            } else {
                cur->reset();
                cur_index = npos;
            }
            length--;
            return *this;
        }

        //pops this node and attempts to retreat to the previous node
        LinkedList& pop_retreat() {
            if (cur_index == npos) return *this;
            Node* cur = &nodes[cur_index];
            Node* next = nullptr;
            Node* prev = nullptr;
            if (cur->next != npos) next = &nodes[cur->next];
            if (cur->prev != npos) prev = &nodes[cur->prev];

            if (prev) {
                //first check if it's the back
                if (cur->next == npos) {
                    swap(cur->next, length > 1 ? 1 : 0);
                }

                //replace references
                prev->next = cur->next;
                if (next) next->prev = cur->prev;

                usize cind = cur_index;
                cur_index = cur->prev;

                cur->reset();

                //move to the back
                swap(cind, length-1);
            } else if (next) {
                //cur must be the first node
                swap(cur->next, 0);


                //we don't have to update prev because its null
                next->prev = npos;

                usize cind = cur_index;
                //only difference if we have to move forward
                cur_index = cur->next;

                cur->reset();

                //move to the back
                swap(cind, length-1);
            } else {
                cur->reset();
                cur_index = npos;
            }
            length--;
            return *this;
        }


        //pops the back of the list, if the current node is the back, it retreats
        LinkedList& pop_back() {
            if (cur_index == npos) return *this;

            usize back_ind = length > 0 /*If empty*/ ? (length >= 2 /*If it has 2 or more elements*/ ? 1:0) : npos;



            Node* back{nullptr};
            if (back_ind != npos) back = &nodes[back_ind];
            else return *this;

            if (back_ind == cur_index) cur_index = back->prev;

            //move this back to the end
            swap(back_ind, length-1);
            back = &nodes[length-1];

            //move its previous node to the "back"
            if (back->prev != npos) swap(back->prev, back_ind);

            back->reset();

            length--;

            return *this;
        }

        //pops the front of the list, if the current node is the front, it advances
        LinkedList& pop_front() {
            if (cur_index == npos) return *this;

            Node* front{nullptr};
            front = &nodes[0];

            if (cur_index == 0) cur_index = front->next;

            //move this front to the end
            swap(0, length-1);
            front = &nodes[length-1];

            //move its previous node to the front
            if (front->next != npos) swap(front->next, 0);

            front->reset();

            length--;

            return *this;
        }


        LinkedList& clear() {
            for (auto& node: nodes) {
                node.reset();
            }

            length = 0;
            cur_index = npos;

            return *this;
        }

        //--------- Navigating List -----------

        void advance() const {
            if (cur_index == npos) return;

            if (nodes[cur_index].next != npos) cur_index = nodes[cur_index].next;
            return;
        }

        void retreat() const {
            if (cur_index == npos) return;

            if (nodes[cur_index].prev != npos) cur_index = nodes[cur_index].prev;
            return;
        }

        void advance(usize amt) const {
            if (cur_index == npos) return;

            while (nodes[cur_index].next != npos && amt-- > 0) {
                cur_index = nodes[cur_index].next;
            }

            return;
        }

        void retreat(usize amt) const {
            if (cur_index == npos) return;

            while (nodes[cur_index].prev != npos && amt-- > 0) {
                cur_index = nodes[cur_index].prev;
            }

            return;
        }

        LinkedList& advance() {
            if (cur_index == npos) return  *this;

            if (nodes[cur_index].next != npos) cur_index = nodes[cur_index].next;
            return *this;
        }

        LinkedList& retreat() {
            if (cur_index == npos) return  *this;

            if (nodes[cur_index].prev != npos) cur_index = nodes[cur_index].prev;
            return  *this;
        }

        LinkedList& advance(usize amt) {
            if (cur_index == npos) return  *this;

            while (nodes[cur_index].next != npos && amt-- > 0) {
                cur_index = nodes[cur_index].next;
            }

            return *this;
        }

        LinkedList& retreat(usize amt) {
            if (cur_index == npos) return *this;

            while (nodes[cur_index].prev != npos && amt-- > 0) {
                cur_index = nodes[cur_index].prev;
            }

            return *this;
        }

        void to_front() const {
            if (cur_index == npos) return;

            cur_index = 0;
            return;
        }

        void to_back() const {
            if (cur_index == npos) return;

            if (length == 1) return;
            cur_index = 1;
        }

        LinkedList& to_front() {
            if (cur_index == npos) return *this;

            cur_index = 0;
            return *this;
        }

        LinkedList& to_back() {
            if (cur_index == npos) return *this;

            if (length == 1) return *this;
            cur_index = 1;
            return *this;
        }

        //--------- Retrieving values -----------

        T& front() const {
            if (length == 0) throw Exception("Cannot access front of empty linked list!");

            return nodes[0].value;
        }

        T& back() const {
            if (length == 0) throw Exception("Cannot access back of empty linked list!");

            usize b = length > 0 /*If empty*/ ? (length >= 2 /*If it has 2 or more elements*/ ? 1:0) : npos;

            return nodes[b].value;
        }

        T& get() const {
            if (cur_index == npos) throw Exception("Cannot access current element of empty linked list!");
            return nodes[cur_index].value;
        }

        //--------- List Data Management ----------

        [[nodiscard]] usize size() const {
            return length;
        }

        [[nodiscard]] bool has_next() const {
            return cur_index != npos && nodes[cur_index].next != npos;
        }

        [[nodiscard]] bool has_prev() const {
            return cur_index != npos && nodes[cur_index].prev != npos;
        }

        [[nodiscard]] bool empty() const {
            return cur_index == npos;
        }

        LinkedList& shrink_to_fit() {
            reserve(length);

            return *this;
        }

        //access the internal Nodes data array
        Array<Node> data() const {
            return nodes;
        }


        usize get_node_index() const {
            return cur_index;
        }

        //--------- Iterators ----------
        //the funny part is a linked list basically is an iterator, for speed however
        //and separation of ideas, ill make a different iterator

        struct Iterator {
        private:
            LinkedList* llist{nullptr};
            usize node_index{npos};

        public:
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;
            using iterator_category = std::random_access_iterator_tag;

            Iterator() = default;

            explicit Iterator(LinkedList& llist) : llist(&llist), node_index(llist.empty() ? npos:0) {}
            explicit Iterator(LinkedList& llist, usize ind) : llist(&llist), node_index(llist.empty() ? npos:ind) {}

            reference operator*() const {
                if (node_index == npos) throw Exception("Invalid access of Linked List via Iterator\n"
                                                        "Note: attempted to dereference a iterator to a empty list");
                return llist->nodes[node_index].value;
            }

            pointer operator->() const {
                if (node_index == npos) return nullptr;
                return &llist->nodes[node_index].value;
            }

            Iterator& operator++() {
                if (node_index == npos) return *this;
                node_index = llist->nodes[node_index].next;
                return *this;
            }

            Iterator operator++(int) {
                if (node_index == npos) return *this;

                Iterator res = *this;
                node_index = llist->nodes[node_index].next;

                return res;
            }

            Iterator& operator--() {
                if (node_index == npos) return *this;
                node_index = llist->nodes[node_index].prev;

                return *this;
            }

            Iterator operator--(int) {
                if (node_index == npos) return *this;

                Iterator res = *this;
                node_index = llist->nodes[node_index].prev;

                return res;
            }

            [[nodiscard]] bool is_valid() const {
                return node_index != npos;
            }

            std::strong_ordering operator<=>(const Iterator &it) const {
                if (it.llist != llist) return llist <=> it.llist;
                return node_index <=> it.node_index;
            }

            bool operator==(const Iterator&) const = default;
            bool operator!=(const Iterator&) const = default;
            bool operator>(const Iterator&) const = default;
            bool operator<(const Iterator&) const = default;
            bool operator>=(const Iterator&) const = default;
            bool operator<=(const Iterator&) const = default;
        };

        Iterator begin() {
            return Iterator(*this);
        }

        Iterator end() {
            return Iterator(*this, npos);
        }

        template<OstreamFormattable U>
        friend std::ostream& operator<<(std::ostream& os, const LinkedList<U>& llist);
        friend ::std::formatter<LinkedList, char>;

    };




    template<OstreamFormattable U>
    std::ostream& operator<<(std::ostream& os, const LinkedList<U>& llist) {
        if (llist.empty()) {
            os << "[]";
            return os;
        }
        os << "[";
        usize node = 0;
        const Array<typename LinkedList<U>::Node>* nodes = &llist.nodes;
        usize max_iterations = llist.size()+1;

        //print all the nodes leading up to the current index
        while (node != llist.get_node_index() && max_iterations > 0) {
            os << nodes->at(node).value;
            node = nodes->at(node).next;
            os << " <- ";
            --max_iterations;
        }

        //print the current node
        os << nodes->at(node).value;
        --max_iterations;
        node = nodes->at(node).next;
        while (node != LinkedList<U>::npos && max_iterations > 0) {
            os << " -> " << nodes->at(node).value;
            node = nodes->at(node).next;
            --max_iterations;
        }
        os << "]";

        return os;
    }





}


template<typename T> requires Auxil::Formattable<T>
struct std::formatter<Auxil::Array<T>, char> {

    std::formatter<T, char> formatter_t;

    template<typename ParseContext>
    auto parse(ParseContext& ctx) {
        return formatter_t.parse(ctx);
    }


    template<typename FormatContext>
    auto format(const Auxil::Array<T>& arr, FormatContext& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "[");

        const auto size = arr.size();
        for (Auxil::usize i = 0; i < size; ++i) {
            out = formatter_t.format(arr[i], ctx);
            if (i != size - 1)
                out = format_to(out, ", ");
        }

        out = format_to(out, "]");
        return out;
    }
};

template<typename T> requires Auxil::Formattable<T>
struct std::formatter<Auxil::Grid<T>, char> {
    std::formatter<T, char> formatter_t{};

    template<typename ParseContext>
    auto parse(ParseContext& ctx) {
        return formatter_t.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const Auxil::Grid<T>& gr, FormatContext& ctx) const {
        using namespace Auxil::Primitives;
        auto out = ctx.out();


        out = std::format_to(out, "[");
        for (usize i = 0; i < gr.rows(); i++) {
            out = std::format_to(out, "{{");

            for (usize j = 0; j < gr.columns(); j++) {
                out = formatter_t.format(gr[i][j], ctx);
                if (j != gr.columns()-1) out = std::format_to(out, ", ");
            }
            out = std::format_to(out, "}}");
            if (i != gr.rows()-1) out = std::format_to(out, ", ");
        }
        out = std::format_to(out, "]");


        return out;
    }
};

template<typename T> requires Auxil::Formattable<T>
struct std::formatter<Auxil::LinkedList<T>, char> {
    std::formatter<T, char> formatter_t{};

    template<typename ParseContext>
    auto parse(ParseContext& ctx) {
        return formatter_t.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const Auxil::LinkedList<T>& llist, FormatContext& ctx) const {
        using namespace Auxil::Primitives;
        auto out = ctx.out();

        if (llist.empty()) {
            return std::format_to(out, "[]");
        }

        out = std::format_to(out, "[");

        const Auxil::Array<typename Auxil::LinkedList<T>::Node>* nodes = &llist.nodes;
        usize node = 0;
        usize max_iterations = llist.size()+1;

        while (node != llist.get_node_index() && max_iterations > 0) {
            out = formatter_t.format(nodes->at(node).value, ctx);
            node = nodes->at(node).next;
            out = std::format_to(out, " <- ");
            --max_iterations;
        }

        //print the current node
        out = formatter_t.format(nodes->at(node).value, ctx);
        --max_iterations;
        node = nodes->at(node).next;
        while (node != Auxil::LinkedList<T>::npos && max_iterations > 0) {
            out = std::format_to(out, " -> ");
            formatter_t.format(nodes->at(node).value, ctx);
            node = nodes->at(node).next;
            --max_iterations;
        }
        out = std::format_to(out, "]");

        return out;
    }
};


#endif