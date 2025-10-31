#ifndef NETWORKING_HPP
#define NETWORKING_HPP
#include <iosfwd>
#include <iostream>
#include <streambuf>
#include "misc.hpp"
#include <boost/asio.hpp>
#include <concepts>
#include "containers.hpp"
#include "str.hpp"



namespace Auxil {
    using boost::asio::ip::tcp;
    using namespace Primitives;

    /*
     * A valid ByteSerializer has a serialize and deserialize function with the following signatures
     * static Array<u8> serialize(const T& data)
     *
     * template<typename F, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<F>, u8>>>
     * static T deserialize(F&& func)
     */
    template<typename T>
    struct ByteSerializer {};

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    struct ByteSerializer<T> {
        static Array<u8> serialize(const T& data) {
            auto result = Array<u8>(sizeof(T));
            //simply copy the bytes into the data, T is a arithmetic type like int so this is fine

            std::memcpy(result.data(), &data, sizeof(T));

            return result;
        }


        template<typename F, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<F>, u8>>>
        static T deserialize(F&& get_callback) {
            std::array<u8, sizeof(T)> data;
            for (auto& b: data) {
                b = get_callback();
            }

            T res;
            std::memcpy(&res, data.data(), sizeof(T));

            return res;
        }
    };

    template<>
    struct ByteSerializer<std::string> {

        static Array<u8> serialize(const std::string& data) {
            Array<u8> res(data.size()+1);
            res.back() = std::bit_cast<u8>('\0');

            std::ranges::copy(data, res.begin());

            return res;
        }


        template<typename F, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<F>, u8>>>
        static std::string deserialize(F&& get_callback) {
            //get to the next '\0'
            std::string result{};
            for (char c = std::bit_cast<char>(get_callback()); c != '\0'; c = std::bit_cast<char>(get_callback())) {
                result.push_back(c);
            }

            return result;
        }
    };


    template<>
    struct ByteSerializer<str> {

        static Array<u8> serialize(const str& data) {
            Array<u8> res(data.size()+1);
            res.back() = static_cast<u8>('\0');

            std::ranges::copy(data, res.begin());

            return res;
        }


        template<typename F, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<F>, u8>>>
        static str deserialize(F&& get_callback) {
            str result{};
            for (char c = std::bit_cast<char>(get_callback()); c != '\0'; c = std::bit_cast<char>(get_callback())) {
                result.push_back(c);
            }

            return result;
        }
    };



    template<typename T>
    concept Serializable = requires(const T data, const std::function<u8()>& bytes)
    {
        { ByteSerializer<T>::serialize(data) } -> std::same_as<Array<u8>>;
        { ByteSerializer<T>::deserialize(bytes) } -> std::convertible_to<T>;
    };

    //a simple skeleton for a server class
    class Server {


    protected:
        //to protect against write operation
        std::mutex mtx;

        boost::asio::io_context context;
        tcp::acceptor acceptor;

    public:
        virtual ~Server() = default;

        Server(const boost::asio::ip::address& ip_addr, const u16 port) :
        acceptor(context, tcp::endpoint(
            ip_addr,
            port
            )) {}

        //copying a server doesnt make sense
        Server(const Server&) = delete;
        Server(Server&& serv) noexcept :
            acceptor(std::move(serv.acceptor)) {}

        Server& operator=(Server&) = delete;
        Server& operator=(Server&& serv) noexcept {
            std::lock_guard lk(mtx);
            //first close our connection
            close();

            //then move the values
            acceptor = std::move(serv.acceptor);

            return *this;
        }


        virtual void close() {
            std::lock_guard lk(mtx);

            acceptor.close();
        }
    };

    //a thread safe client
    class Client {
        boost::asio::io_context io;
        tcp::resolver resolver;
        tcp::socket socket;
        std::mutex mtx;


        std::array<u8, 1024> buffer{0};
        usize buf_pos{0};
        usize data_size{0};

        atomic_bool has_connection = true;

        template<typename ErrorResolver, typename = std::enable_if_t<std::invocable<ErrorResolver, boost::system::error_code>>>
        void connect(const tcp::resolver::results_type& endpoints, ErrorResolver&& resolver) {
            boost::system::error_code ec;
            boost::asio::connect(socket, endpoints, ec);

            if (ec) {
                resolver(ec);
            }
        }

        void get_data() {
            std::lock_guard lk(mtx);
            get_data_unlocked();
        }

        void get_data_unlocked() {
            if (!has_connection) throw Exception("Cannot read data from unconnected server");
            boost::system::error_code ec;
            data_size = socket.read_some(boost::asio::buffer(buffer), ec);
            buf_pos = 0;

            if (ec) {
                throw Exception("Failed to read data with error: {}", ec.message());
            }
        }

        u8 get_byte_unlocked() {
            if (buf_pos == data_size) get_data_unlocked();

            return buffer[buf_pos++];
        }


    public:
        Client(boost::asio::ip::address ip_addr, const u16 port) : resolver(io), socket(io) {
            const tcp::resolver::results_type endpoints = resolver.resolve(ip_addr.to_string(), std::to_string(port));

            connect(endpoints, [ip_addr, port](const boost::system::error_code& ec) {
                throw Exception("Failed to connect to \"{}:{}\": {}", ip_addr.to_string(), port, ec.message());
            });
        }

        Client(boost::asio::ip::address ip_addr, const std::string_view service) : resolver(io), socket(io) {
            tcp::resolver::results_type endpoints = resolver.resolve(ip_addr.to_string(), service);

            connect(endpoints, [ip_addr, service](const boost::system::error_code& ec) {
                throw Exception("Failed to connect to {} with service {}: {}", ip_addr.to_string(), service, ec.message());
            });
        }

        Client(const Client&) = delete;
        Client(Client&& c)  noexcept :
        resolver(std::move(c.resolver)),
        socket(std::move(c.socket)),
        buffer(c.buffer),
        buf_pos(c.buf_pos),
        data_size(c.data_size),
        has_connection(c.has_connection.load()) {
            c.has_connection = false;
            c.data_size = 0;
            c.buf_pos = 0;
        }

        Client& operator=(const Client&) = delete;
        Client& operator=(Client&& c)  noexcept {
            this->close();
            std::lock_guard lk(mtx);

            resolver = std::move(c.resolver);
            socket = std::move(c.socket);
            buffer = c.buffer;
            buf_pos = c.buf_pos;
            data_size = c.data_size;
            has_connection = c.has_connection.load();

            c.has_connection = false;
            c.data_size = 0;
            c.buf_pos = 0;

            return *this;
        }

        [[nodiscard]] bool connected() const {
            return has_connection;
        }


        void close() {
            if (!has_connection) return;
            std::lock_guard lk(mtx);
            socket.close();

            has_connection = false;
        }

        void connect(const char* ip, const u16 port) {
            //first close the current connection
            if (has_connection) close();

            std::lock_guard lk(mtx);

            const tcp::resolver::results_type endpoints = resolver.resolve(ip, std::to_string(port));

            connect(endpoints, [ip, port](const boost::system::error_code& ec) {
                throw Exception("Failed to connect to \"{}:{}\": {}", ip, port, ec.message());
            });
        }

        void connect(const char* ip, const std::string_view& service) {
            //first close the current connection
            if (has_connection) close();

            std::lock_guard lk(mtx);

            tcp::resolver::results_type endpoints = resolver.resolve(ip, service);

            connect(endpoints, [ip, service](const boost::system::error_code& ec) {
                throw Exception("Failed to connect to {} with service {}: {}", ip, service, ec.message());
            });
        }

        //writes to the server, blocks until the message has been sent
        template<Serializable T>
        void write(const T& data) {
            std::lock_guard lk(mtx);
            if (!has_connection) throw Exception("Cannot write data to unconnected server");

            auto bytes = ByteSerializer<T>::serialize(data);

            boost::asio::write(socket, boost::asio::buffer(bytes.data(), bytes.size()));
        }

        //gets the next byte of data from the server, potentially reads in up to 1024 bytes
        //if the internal buffer runs out, in such a case it would block until it reads something
        u8 get_next_byte() {
            std::lock_guard lk(mtx);
            return get_byte_unlocked();
        }

        Array<u8> get_n_bytes(const usize n) {
            std::lock_guard lk(mtx);

            Array<u8> result(n);

            for (usize i = 0; i < n; i++) {
                if (buf_pos == data_size) get_data_unlocked();

                result[i] = buffer[buf_pos++];
            }

            return result;
        }

        //reads in a T, T must have a defined ByteSerializer
        template<Serializable T>
        T read() {
            //this ensures the serializer is not interupted by another reader
            //causing issues where it reads every other byte instead of
            //a continuous stream of bytes
            std::lock_guard lk(mtx);
            return ByteSerializer<T>::deserialize([this]() -> u8 {
                return get_byte_unlocked();
            });
        }

        //writes the data asynchronously
        template<Serializable T>
        std::future<void> write_async(const T& data) {
            return std::async(std::launch::async, &Client::write<T>, this, data);
        }

        std::future<u8> async_next_byte() {
            return std::async(std::launch::async, &Client::get_next_byte, this);
        }

        std::future<Array<u8>> async_n_bytes(const usize n) {
            return std::async(std::launch::async, &Client::get_n_bytes, this, n);
        }

        template<Serializable T>
        std::future<T> read_async() {
            return std::async(std::launch::async, &Client::read<T>, this);
        }
    };

    //a thread-safe server that only talks to one client at a time,
    //closes the connection with that client when done or when it wants to accept a new connection
    //implemented as a sort of 'stream'
    class SingleServer final : public Server {
    protected:
        tcp::socket client;
        atomic_bool has_connection{false};
        std::array<u8, 1024> buffer{0};
        usize buf_pos{0};
        usize data_size{0};

        void get_data() {
            std::lock_guard lk(mtx);
            if (!has_connection) throw Exception("Cannot read data from unconnected client");
            boost::system::error_code ec;
            data_size = client.read_some(boost::asio::buffer(buffer), ec);
            buf_pos = 0;

            if (ec) {
                throw Exception("Failed to read data with error: {}", ec.message());
            }
        }

        void get_data_unlocked() {
            if (!has_connection) throw Exception("Cannot read data from unconnected client");
            boost::system::error_code ec;
            data_size = client.read_some(boost::asio::buffer(buffer), ec);
            buf_pos = 0;

            if (ec) {
                throw Exception("Failed to read data with error: {}", ec.message());
            }
        }

    public:
        SingleServer(const boost::asio::ip::address &ip_addr, const u16 port) : Server(ip_addr, port), client(context) {}

        SingleServer(SingleServer&& serv) noexcept : Server(std::move(serv)), client(std::move(serv.client)) {}

        [[nodiscard]] bool connected() const {
            return has_connection;
        }

        SingleServer& operator=(SingleServer&& serv) noexcept {
            client = std::move(serv.client);

            Server::operator=(std::forward<SingleServer>(serv));

            return *this;
        }

        //accepts a new connection, blocks until something has been accepted
        void accept() {
            //this waits for all write operations to finish
            std::lock_guard lk(mtx);

            //first close our current connection
            boost::system::error_code ec;
            if (has_connection) client.close(ec);
            if (ec) {
                throw Exception("Client connection did not shutdown correctly: {}", ec.message());
            }
            has_connection = false;

            //accept another connection
            acceptor.accept(client);
            has_connection = true;
        }

        //writes to the server, blocks until the message has been sent
        template<Serializable T>
        void write(const T& data) {
            std::lock_guard lk(mtx);
            if (!has_connection) throw Exception("Cannot write data to unconnected client");

            auto bytes = ByteSerializer<T>::serialize(data);

            boost::asio::write(client, boost::asio::buffer(bytes.data(), bytes.size()));
        }

        //gets the next byte of data from the server, potentially reads in up to 1024 bytes
        //if the internal buffer runs out, in such a case it would block until it reads something
        u8 get_next_byte() {
            std::lock_guard lk(mtx);
            //fine to use the unlocked version since we own a lock
            if (buf_pos == data_size) get_data_unlocked();

            return buffer[buf_pos++];
        }

        Array<u8> get_n_bytes(const usize n) {
            std::lock_guard lk(mtx);

            Array<u8> result(n);

            for (usize i = 0; i < n; i++) {
                if (buf_pos == data_size) get_data_unlocked();

                result[i] = buffer[buf_pos++];
            }

            return result;
        }

        //reads in a T, T must have a defined ByteSerializer
        template<Serializable T>
        T read() {
            return ByteSerializer<T>::deserialize([this]() -> u8 {
                return get_next_byte();
            });
        }

        //writes the data asynchronously
        template<Serializable T>
        std::future<void> write_async(const T& data) {
            return std::async(std::launch::async, &SingleServer::write<T>, this, data);
        }

        std::future<u8> async_next_byte() {
            return std::async(std::launch::async, &SingleServer::get_next_byte, this);
        }

        std::future<Array<u8>> async_n_bytes(const usize n) {
            return std::async(std::launch::async, &SingleServer::get_n_bytes, this, n);
        }

        template<Serializable T>
        std::future<T> read_async() {
            return std::async(std::launch::async, &SingleServer::read<T>, this);
        }
    };


    class MultiServer final : public Server {
    public:
        //describes a client connected to the server, allows you to read/write from the specific client
        struct Client {
            //makes a basic client
            Client() = default;
        private:
            atomic_bool connected{false};
            std::unique_ptr<tcp::socket> client_socket{nullptr};

            explicit Client(tcp::socket&& socket) :
                connected(true),
                client_socket(std::make_unique<tcp::socket>(std::forward<tcp::socket>(socket)))
            { }

            std::mutex mtx;


            std::array<u8, 1024> buffer{0};
            usize buf_pos{0};
            usize data_size{0};

            void get_data() {
                std::lock_guard lk(mtx);
                get_data_unlocked();
            }

            void get_data_unlocked() {
                if (!connected || !client_socket) throw Exception("Cannot read data from unconnected server");
                boost::system::error_code ec;
                data_size = client_socket->read_some(boost::asio::buffer(buffer), ec);
                buf_pos = 0;

                if (ec) {
                    throw Exception("Failed to read data with error: {}", ec.message());
                }
            }

            u8 get_byte_unlocked() {
                if (buf_pos == data_size) get_data_unlocked();

                return buffer[buf_pos++];
            }
        public:
            friend MultiServer;

            Client(const Client&) = delete;
            Client& operator=(const Client&) = delete;

            Client(Client&& c) noexcept :
                connected(c.connected.load()),
                client_socket(std::move(c.client_socket)),
                buffer(c.buffer),
                buf_pos(c.buf_pos),
                data_size(c.data_size)
            {
                c.buf_pos = 0;
                c.data_size = 0;
            }

            Client& operator=(Client&& c)  noexcept {
                close();

                std::lock_guard lk(mtx);
                connected = c.connected.load();
                client_socket = std::move(c.client_socket);
                buffer = c.buffer;
                buf_pos = c.buf_pos;
                data_size = c.data_size;
                c.buf_pos = 0;
                c.data_size = 0;

                return *this;
            }


            [[nodiscard]] bool is_connected() {
                connected = client_socket->is_open();
                return connected;
            }


            void close() {
                std::lock_guard lk(mtx);
                client_socket->close();

                connected = false;
            }

            //writes to the server, blocks until the message has been sent
            template<Serializable T>
            void write(const T& data) {
                std::lock_guard lk(mtx);
                if (!connected || !client_socket) throw Exception("Cannot write data to unconnected server");

                auto bytes = ByteSerializer<T>::serialize(data);

                boost::asio::write(*client_socket, boost::asio::buffer(bytes.data(), bytes.size()));
            }

            //gets the next byte of data from the server, potentially reads in up to 1024 bytes
            //if the internal buffer runs out, in such a case it would block until it reads something
            u8 get_next_byte() {
                std::lock_guard lk(mtx);
                return get_byte_unlocked();
            }

            Array<u8> get_n_bytes(const usize n) {
                std::lock_guard lk(mtx);

                Array<u8> result(n);

                for (usize i = 0; i < n; i++) {
                    if (buf_pos == data_size) get_data_unlocked();

                    result[i] = buffer[buf_pos++];
                }

                return result;
            }

            //reads in a T, T must have a defined ByteSerializer
            template<Serializable T>
            T read() {
                //this ensures the serializer is not interrupted by another reader
                //causing issues where it reads every other byte instead of
                //a continuous stream of bytes
                std::lock_guard lk(mtx);
                return ByteSerializer<T>::deserialize([this]() -> u8 {
                    return get_byte_unlocked();
                });
            }

            //writes the data asynchronously
            template<Serializable T>
            std::future<void> write_async(const T& data) {
                return std::async(std::launch::async, &Client::write<T>, this, data);
            }

            std::future<u8> async_next_byte() {
                return std::async(std::launch::async, &Client::get_next_byte, this);
            }

            std::future<Array<u8>> async_n_bytes(const usize n) {
                return std::async(std::launch::async, &Client::get_n_bytes, this, n);
            }

            template<Serializable T>
            std::future<T> read_async() {
                return std::async(std::launch::async, &Client::read<T>, this);
            }
        };
    private:
    public:
        using Server::Server;
        using Server::operator=;

        //accepts a new client connection
        Client accept() {
            tcp::socket sk = acceptor.accept();

            return Client{ std::move(sk) };
        }
    };
}

#endif