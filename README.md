# Auxil
My latest utilities library for C++, contains utilities focused around adding features C++ lacks/has bad support for and game development
Most stuff is well-tested, maybe not for every edge case, but for at least the most common ones

# Latest Patch Notes
 - The LinkedList::pop_* methods now return the popped element and throw an exception for an empty list
 - Removed the threading library, it needs some major work...


# Stats
| Name | Value |
| :--: | :--: |
| Total lines | 4,308 |
| Empty lines | 1,015 |
| Total non-empty lines | 3,293 |
| Total chars | 122,059 |

# Sub-libraries list
| Name | Description |
| :--: | :---------: |
| **Containers** | Contains container data structures |
| **Exception** | uses Boost::Stacktrace and formatting to make better exceptions |
| **Globals** | Globals used by several components of the library |
| **Iterator** | Utilites for iterators |
| **Math** | Contains functions and structures for mathematical tasks |
| **Misc** | Contains simple utilities |
| **Print** | Uses modern formatting to add support for better printing and runtime formatting (To be deprecated when full support for std::print(ln) is available |
| **Random** | The random class for better RNG |
| **Threading** | Utilites for multi-threading operations |


# Containers

**Class Array<std::semiregular T>**
* A fixed-size runtime-allocated container, useful in place of using new[]/delete[] *

**Methods**

| Method | Description |
| :----: | :---------: |
| `~Array()` | Destructor |
| `Array()` | Default initializer |
| `Array(usize size)` | Creates an Array with `size` elements |
| `Array(T* ptr, usize size)` | Wraps an array around a existing pointer, does not auto-free the pointer |
| `Array(const std::initializer_list<T>& init)` | Creates an array from a initializer_list of elements |
| `Array(const Iterable& iterable)` | Creates an array from a iterable container |
| copy/move constructors and operators | Creates/Sets the Array from an existing array |
| `bool is_pointer_wrapper()` | Returns if the array wraps a pointer rather than owning it |
| `bool empty()` | Returns if the array has 0 elements |
| `usize size()` | Returns the size of the array |
| `begin()` | Returns a iterator to the start of the array |
| `end()` | Returns a iterator to the end of the array |
| `rbegin()` | Returns a reverse iterator to the end of the array |
| `rend()` | Returns a reverse iterator to the start of the array |
| `T& front()` | Returns the first element |
| `T& back()` | Returns the last element |
| `T& at(usize ind)` | returns the element at the specified index |
| `T& operator [usize ind]` | returns the element at the specified index |
| `T& emplace_at(usize ind, Args&& constructor)` | Constructs an element in-place at the specified index, destructing the old element |
| `T* data()` | Returns the internal pointer, deleting this pointer may lead to undefined behavior |

| Non-Members | Description |
| :---------: | :---------: |
| `std::ostream& operator<<` | defines a operator for printing the Array with a std::ostream, only works if the element also has a defined operator |
| `std::formatter<Array>` | Defines a formatter for formatting the Array |

**Class Grid<std::semiregular T>**
* A fixed-size continuous-runtime-allocated matrix, useful instead of something like Array<Array<T>> with support for matrix operations (so long as T has the correct operators, otherwise using said operators wont compile) *

| Method | Description |
| :----: | :---------: |
| `~Grid()` | Destructor |
| `Grid()` | Default Constructor |
| `Grid(std::initalizer_list<std::initializer_list<T>>)` | Construtor from initializer matrix |
| copy/move constructors and operators | Creates/Sets the Grid from an existing Grid |
| `T& emplace_at(usize row, usize columns, Args&&... constructor)` | Creates a new element in-place at the specified coordinate, destructing the old one |
| `usize width()` | Returns the width (or number of columns) of the grid |
| `usize height()` |Returns the height (or number of rows) of the grid |
| `usize area()` | Returns the total area of the grid (total number of elements) |
| `usize columns()` | Returns the number columns (or width) of the grid |
| `usize rows()` | Returns the number of rows (or height) of the grid |
| `usize size()` | Returns the total number of elements in the grid (or area) |
| `Array<T> at()` | Returns a pointer wrapped by Array<T> of the data in the specified row |
| `T& at_flat()` | Returns the element at the specified index as if though the data were a continuous array |
| `Array<T> operator[usize ind]` | Returns a pointer wrapped by Array<T> of the data in the specified row |
| `Array<T> front()` | Returns the first row |
| `Array<T> back()` | Returns the last row |
| `T& first()` | Returns the first element |
| `T& last()` | Returns the last element |
| `T* data()` | Returns the internal pointer array of elements, deleting this pointer may lead to undefined behavior |
| `begin()` | Returns a iterator to the start of the grid |
| `end()` | Returns a iterator to the end of the grid |
| `rbegin()` | Returns a reverse iterator to the end of the grid |
| `rend()` | Returns a reverse iterator to the start of the grid |
| `bool empty()` | Returns if the grid has no elements |
| `Grid& reset()` | Resets all values to their default value (calls T()) |
| `operator +/-` | If the other matrix is of the same dimensions, it adds that matrixes values to the current one, if using the op= variant, it adds them in-place |
| `Grid dot(const Grid& right)` | Computes and returns the dot product between the 2 grids |
| `operator *` | Computes and returns the dot product between the 2 grids |
| `operator *=` | Computes a hadamard multiplication on the 2 matrices and stores in-place |
| `Grid multiply` | Computes a hadamard multiplication on the 2 matrices and returns it |
| `operator /(Arithmetic scalar)` | divides the whole matrix by the scalar |




| Non-Members | Description |
| :---------: | :---------: |
| `Grid::make(usize rows, usize columns)` | creates a grid with `rows` rows and `columns` columns |
| `std::ostream& operator<<` | Outputs the grid to the ostream as long as the underlying type has a defined output operator |
| `std::formatter<Grid<T>>` | Makes the grid available to be formatted via std::format and the such |



**Class LinkedList<std::semiregular T>**
* A dynamically-sized LinkedList that stores nodes continuously and keeps track of the start and end indices, combining the speed of insertion/popping of a linked list without the overhead of heavy heap allocations *

| Method | Description |
| :----: | :---------: |
| `~LinkedList()` | Destructor |
| `LinkedList()` | Default constructor |
| `LinkedList(std::initializer_list<T>)` | Constructs from elements in the initializer list |
| copy/move constructors and operators | Creates/Sets the LinkedList from an existing LinkedList |
| `LinkedList& reserve(usize n)` | Reserves memory to store `n` elements but does not add new nodes |
| `LinkedList& resize(usize n)` | Adds `n` new default nodes |
| `T& push_back(const T& value)` | Appends the value to the back of the list |
| `T& push_front(const T& value)` | Appends the value to the front of the list |
| `T& push_back(const T&& value)` | Appends the value via move to the back of the list |
| `T& push_front(const T&& value)` | Appends the value via move to the front of the list |
| `T& push_ahead(const T& value)` | Appends the value ahead of the current node |
| `T& push_behind(const T& value)` | Appends the value behind the current node |
| `T& push_ahead(const T&& value)` | Appends the value via move ahead of the current node |
| `T& push_behind(const T&& value)` | Appends the value via move behind the current node |
| `LinkedList& pop_ahead()` | Pops the value ahead of the current node |
| `LinkedList& pop_behind()` | Pops the value behind of the current node |
| `LinkedList& pop_advance()` | Pops the current node and tries to advance forwards |
| `LinkedList& pop_retreat()` | Pops the current node and tries to advance backwards |
| `LinkedList& pop_back()` | Pops the back value |
| `LinkedList& pop_front()` | Pops the front value |
| `LinkedList& clear()` | Clears all nodes |
| `LinkedList& advance(usize n = 1)` | Advances `n` nodes |
| `LinkedList& retreat(usize n = 1)` | Retreats `n` nodes |
| `void advance(usize n = 1) const` | Advances `n` nodes (const version) |
| `void retreat(usize n = 1) const` | Retreats `n` nodes (const version) |
| `void to_front() const` | Gos to the front of the list (const version) |
| `void to_back() const` | Gos to the back of the list (const version) |
| `LinkedList& to_front() const` | Gos to the front of the list |
| `LinkedList& to_back() const` | Gos to the back of the list |
| `T& front()` | Returns the front element |
| `T& back()` | Returns the back element |
| `T& get()` | Returns the current nodes element |
| `usize size()` | Returns the lists size |
| `bool has_next()` | Returns if theres a node ahead of the current one |
| `bool has_prev()` | Returns if theres a node behind the current one |
| `bool empty()` | Returns if the list is empty |
| `LinkedList& shrink_to_fit()` | Reduces memory usage by shrinking the list to fit only the number of nodes it has |
| `Array<Node> data()` | Returns a copy of the nodes Array |
| `usize get_node_index()` | Returns the nodes current index into the array, or npos if the array is empty |
| `begin()` | Returns a iterator to the first node |
| `end()` | Returns a iterator to the last node |



| Non-Members | Description |
| :---------: | :---------: |
| `usize npos` | a value representing an invalid index |
| `LinkedList::Node` | The node structure used internally by the linked list |
| `LinkedList::Iterator` | A custom iterator for iterating through the linked list |
| `std::ostream& operator<<` | Defines an operator for printing the linked list |
| `std::formatter<LinkedList<T>>` | Defines a formatter for the linked list |



# Exception

**Class Exception**
* A Exception class allowing you to print formatted error messages and combines with the boost::stacktrace library for stacktraces *

| Method | Description |
| :----: | :---------: |
| `Exception()` | Default constructor |
| `Exception(const char* message)` | Simple string constructor |
| `Exception(const std::string& format, Args&&... args)` | Creates a formatted exception |
| `const char* what()` | Returns the error message |
| `boost::stacktrace::stacktrace` | Returns the stacktrace |


#  Globals

*Maybe one day there will be more stuff in this section*
`#define FORCE_INLINE __attribute__((always_inline)) inline`


# TODO: Iterator, Math, Misc, Print, Random, Threading
**These dont have documentation because most of them are several hundered if not thousand lines of code long and im tired**

