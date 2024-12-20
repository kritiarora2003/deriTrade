#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include "nlohmann/json.hpp" // Include the JSON library for parsing
#include "credentials.h"     // Replace with your credentials header

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

std::mutex cout_mutex;  // Mutex to prevent console output conflicts
std::queue<std::string> symbol_queue; // Queue for user-submitted symbols
bool exit_flag = false;

// Function to display formatted JSON
void print_formatted_json(const std::string& raw_json) {
    try {
        json parsed = json::parse(raw_json);
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "-----------------------------------" << std::endl;
        std::cout << parsed.dump(4) << std::endl; // Indented JSON
        std::cout << "-----------------------------------" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
    }
}

// Function to send subscription requests for symbols
void subscribe_to_symbol(client* c, websocketpp::connection_hdl hdl, const std::string& symbol) {
    json payload = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "public/subscribe"},
        {"params", {{"channels", {"book." + symbol + ".raw"}}}}
    };

    websocketpp::lib::error_code ec;
    c->send(hdl, payload.dump(), websocketpp::frame::opcode::text, ec);
    if (ec) {
        std::cout << "Failed to subscribe to symbol " << symbol << ": " << ec.message() << std::endl;
    } else {
        std::cout << "Subscribed to symbol: " << symbol << std::endl;
    }
}

// Function to handle WebSocket opening
void on_open(websocketpp::connection_hdl hdl, client* c) {
    std::cout << "WebSocket connection opened!" << std::endl;

    websocketpp::lib::error_code ec;

    // Authenticate with Deribit API
    json auth_payload = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "client_credentials"},
            {"client_id", Credentials::getClientId()},
            {"client_secret", Credentials::getClientSecret()}
        }}
    };

    c->send(hdl, auth_payload.dump(), websocketpp::frame::opcode::text, ec);
    if (ec) {
        std::cout << "Failed to send authentication payload: " << ec.message() << std::endl;
    } else {
        std::cout << "Authentication payload sent." << std::endl;
    }

    // Start a separate thread to handle user input for subscriptions
    std::thread([c, hdl]() {
        while (!exit_flag) {
            std::string symbol;
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Enter a symbol to subscribe (e.g., BTC-PERPETUAL), or type 'exit' to quit: ";
                std::cin >> symbol;
            }
            if (symbol == "exit") {
                exit_flag = true;
                break;
            }
            symbol_queue.push(symbol);
        }
    }).detach();
}

// Function to handle WebSocket messages
void on_message(websocketpp::connection_hdl hdl, client* c, client::message_ptr msg) {
    std::string payload = msg->get_payload();

    // Print formatted JSON for every message
    print_formatted_json(payload);

    // Process symbols in the queue for subscription
    while (!symbol_queue.empty()) {
        std::string symbol = symbol_queue.front();
        symbol_queue.pop();
        subscribe_to_symbol(c, hdl, symbol);
    }

    // Handle real-time order book data
    try {
        json response = json::parse(payload);

        if (response.contains("params") && response["params"].contains("data")) {
            json data = response["params"]["data"];
            std::string symbol = data.value("instrument_name", "Unknown");

            // Print real-time order book data (bids and asks)
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\nReal-time order book data for " << symbol << ":\n";
            
            if (data.contains("bids")) {
                std::cout << "Bids:\n";
                for (const auto& bid : data["bids"]) {
                    std::cout << "  Price: " << bid[0] << ", Size: " << bid[1] << "\n";
                }
            }

            if (data.contains("asks")) {
                std::cout << "Asks:\n";
                for (const auto& ask : data["asks"]) {
                    std::cout << "  Price: " << ask[0] << ", Size: " << ask[1] << "\n";
                }
            }
            std::cout << "------------------------------------------\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}

// Function to handle WebSocket failures
void on_fail(websocketpp::connection_hdl hdl) {
    std::cout << "WebSocket connection failed!" << std::endl;
}

// Function to handle WebSocket closures
void on_close(websocketpp::connection_hdl hdl) {
    std::cout << "WebSocket connection closed!" << std::endl;
}

// Function to initialize TLS
context_ptr on_tls_init(const char* hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
        ctx->set_verify_mode(boost::asio::ssl::verify_none);
    } catch (std::exception& e) {
        std::cout << "TLS Initialization Error: " << e.what() << std::endl;
    }
    return ctx;
}

int main() {
    client c;

    std::string hostname = "test.deribit.com"; // Use "test.deribit.com" for the test environment
    std::string uri = "wss://" + hostname + "/ws/api/v2";

    try {
        // Configure WebSocket++ client
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);
        c.init_asio();

        // Set handlers with correctly qualified placeholders
        c.set_message_handler(std::bind(&on_message, std::placeholders::_1, &c, std::placeholders::_2));
        c.set_tls_init_handler(std::bind(&on_tls_init, hostname.c_str(), std::placeholders::_1));
        c.set_open_handler(std::bind(&on_open, std::placeholders::_1, &c));
        c.set_fail_handler(std::bind(&on_fail, std::placeholders::_1));
        c.set_close_handler(std::bind(&on_close, std::placeholders::_1));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "Could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        // Connect and run the WebSocket client
        c.connect(con);
        c.run();
    } catch (websocketpp::exception const& e) {
        std::cout << "WebSocket Exception: " << e.what() << std::endl;
    }

    return 0;
}
