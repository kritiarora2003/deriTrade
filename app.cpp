#include <iostream>
#include <string>
#include <fstream>  // For file operations
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <iomanip> 
#include "credentials.h"

// Using nlohmann::json
using json = nlohmann::json;
using namespace std;


// class myOrder {
//     // myorder ke andar kya kya hona chahiye
//     // order id
//     // amount
//     // price
//     // instrument name
//     // order state
//     // etc
// };

// helper functions
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
string displayAndChoose(const vector<string>& options, const string& type);
void putInJSONfile(json response_json);
bool getAPIResponse();
string curlRequest(const string& url, const json& msg_request, const string& password = "");
void displayMenu();

// list of all the user functions
string authenticate(const string& clientId, const string& clientSecret);
void buy(string password, string instrument_name, string amount, const string& id = "");
void sell(const string& password, const string& instrument_name, const string& amount, const string& price, const string& id = "");
void cancelOrder(const string& password, const string& orderid, const string& id = "");
void getOrderBook(const string& instrument_name, const string& id = "");
void modifyOrder(const string& password, const string& orderid, const string& amount, const string& price, const string& id);
void getOpenOrders(const string& password, const string& id = "");
void getPosition(const string& password, const string& currency, const string& kind, const string& id= "");




// This function will handle the HTTP response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    ((std::string*)userp)->append((char*)contents, totalSize);
    return totalSize;
}


string displayAndChoose(const vector<string>& options, const string& type) {
    // Display the list of options
    cout << "Available " << type << "s:" << endl;
    for (size_t i = 0; i < options.size(); ++i) {
        cout << i + 1 << ". " << options[i] << endl;
    }

    // Ask the user to choose an option
    cout << "\nEnter the number of the " << type << " you want to choose: ";
    int choice;
    cin >> choice;

    // Validate user input
    if (choice < 1 || choice > options.size()) {
        throw invalid_argument("Invalid choice! Please choose a valid " + type + ".");
    }

    // Set the user's choice
    return options[choice - 1];
}

// function to put stuff in json files, for testing purposes
void putInJSONfile(json response_json) {
    ofstream outputFile("response.json");
    if (outputFile.is_open()) {
        outputFile << response_json.dump(4);  // 4 spaces for pretty printing
        outputFile.close();
        cout << "JSON response written to 'response.json'" << endl;
    } 
    else {
        cerr << "Failed to open file for writing." << endl;
    }
}

// This function sends the API request to Deribit 
bool getAPIResponse() {
    CURL* handler = curl_easy_init(); 
    CURLcode res;
    string readBuffer;
    bool APIworking;

    curl_global_init(CURL_GLOBAL_DEFAULT);  // Initialize libcurl
     // Initialize a curl session

    if (handler) {
        // testing different urls to learn more about the api
        // const char* url = "https://api.thecatapi.com/v1/breeds";
        // const char* url = "https://test.deribit.com/api/v2/public/get_order_book";
        // const char* url = "https://test.deribit.com/api/v2/public/get_order_book?instrument_name=BTC-PERPETUAL";
        // const char* url = "https://test.deribit.com/api/v2/public/get_time";
        // const char* url = "https://test.deribit.com/api/v2/public/status";
        // const char* url = "https://test.deribit.com/api/v2/public/test";
        // const char* url = "https://test.deribit.com/api/v2/public/hello";
        const char* url = "https://test.deribit.com/api/v2/public/get_currencies";

        // Set options for the HTTP request
        curl_easy_setopt(handler, CURLOPT_URL, url);
        curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(handler, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(handler);

        if (res != CURLE_OK) {
            cerr << "cURL request failed: " << curl_easy_strerror(res) << endl;
            APIworking = 0;
        } 
        else {
            APIworking = 1;
            // Parse the JSON response
            try {

                // string to json parsing funciton in c++
                json response_json = json::parse(readBuffer);

                // Write the JSON to a file
                putInJSONfile(response_json);

            } catch (json::parse_error& e) {
                cerr << "Error parsing JSON: " << e.what() << endl;
            }
        }

        curl_easy_cleanup(handler);  // Cleanup the curl session
    }

    curl_global_cleanup();  // Cleanup global resources
    return APIworking;
}

string curlRequest(const string& url, const json& msg_request, const string& password) {
    CURL* curl;
    CURLcode res;
    string response_string;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

        if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        string data = msg_request.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());


        // Set headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        if (!password.empty()) {
            headers = curl_slist_append(headers, ("Authorization: Bearer " + password).c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Perform the request
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)  {
            std::cerr << "Curl request failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
    return response_string;
}

string authenticate(const string& clientId, const string& clientSecret) {
    CURL* curl;
    CURLcode res;
    string token;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://test.deribit.com/api/v2/public/auth");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Prepare JSON data
        json json_data = {
            {"method", "public/auth"},
            {"params", {
                {"grant_type", "client_credentials"},
                {"client_id", clientId},
                {"client_secret", clientSecret}
            }}
        };

        string data = json_data.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

        // Set headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Perform the request
        string response_string;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            // Print raw response for debugging
            // std::cout << "Response: " << response_string << std::endl;

            try {
                // Parse the response as JSON
                auto json_response = nlohmann::json::parse(response_string);

                putInJSONfile(json_response);
                // Check if the access token is present in the response
                if (json_response.contains("result") && json_response["result"].contains("access_token")) {
                    token = json_response["result"]["access_token"];
                } else {
                    std::cerr << "Error: Access token not found in the response." << std::endl;
                }
            } catch (const nlohmann::json::exception& e) {
                std::cerr << "JSON Parsing error: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Curl request failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    return token;
}

void buy(string password, string instrument_name, string amount, const string& id) {

    json json_data = {
        {"jsonrpc" , "2.0"},
        {"id", id},
        {"method", "private/buy"},
        {"params" , {
            {"instrument_name", instrument_name},
            {"amount" , amount},
            {"type", "market"},
            {"label" , "market0000234"}
        }}
    };

    const string url = "https://test.deribit.com/api/v2/private/buy";

    string response = curlRequest(url, json_data, password);
    
    auto json_response = nlohmann::json::parse(response);
    std::cout << "Response: " << response << std::endl;

    putInJSONfile(json_response);
}

void sell(const string& password, const string& instrument_name, const string& amount, const string& price, const string& id){
    json json_data = {
        {"jsonrpc" , "2.0"},
        {"method", "private/sell"},
        {"params" , {
            {"instrument_name", instrument_name},
            {"amount" , amount},
            {"price", price},
            {"type", "limit"},
        }}
    };

    const string url = "https://test.deribit.com/api/v2/private/sell";

    string response = curlRequest(url, json_data, password);
    
    auto json_response = nlohmann::json::parse(response);
    std::cout << "Response: " << response << std::endl;

    putInJSONfile(json_response);
}

void cancelOrder(const string& password, const string& orderid, const string& id) {
    json request = {
        {"jsonrpc" , "2.0"},
        {"id" , id},
        {"method" , "private/cancel"},
        {"params" , {
            {"order_id", orderid}
        }}
    };   

    const string url = "https://test.deribit.com/api/v2/private/cancel";

    string response = curlRequest(url, request, password);

    try {
        // Parse the JSON response
        auto json_response = nlohmann::json::parse(response);
        putInJSONfile(json_response);

        if(json_response.contains("result")) {
            cout<<"Order cancelled successfuly, printing updated orders\n";
            getOpenOrders(password);
        }
        else if (json_response.contains("error"))
            cout<<"Error in cancelling order, check inputted order ID\n";
    } catch (const exception& e) {
        cerr << "Error parsing or processing the JSON response: " << e.what() << endl;
    }
}

void getOrderBook(const string& instrument_name, const string& id) {
    const string& url = "https://test.deribit.com/api/v2/public/get_order_book";
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "public/get_order_book"},
        {"params", {
            {"instrument_name", instrument_name}, 
            {"depth", 5}
            }
        }
    };

    string response = curlRequest(url, request);
    
    try {
        // Parse the JSON response
        auto json_response = nlohmann::json::parse(response);
        putInJSONfile(response);
        // Extract the "result" object
        if (!json_response.contains("result") || !json_response["result"].is_object()) {
            throw runtime_error("Invalid or missing 'result' object in response");
        }

        auto result = json_response["result"];

        // Print general information
        cout << "Market Data for Instrument: " << result["instrument_name"] << endl;
        cout << "State: " << result["state"] << endl;

        // Print key prices
        cout << "\nKey Prices:" << endl;
        cout << "  Last Price: " << result["last_price"] << endl;
        cout << "  Mark Price: " << result["mark_price"] << endl;
        cout << "  Settlement Price: " << result["settlement_price"] << endl;
        cout << "  Index Price: " << result["index_price"] << endl;

        // Print min/max prices
        cout << "\nPrice Range:" << endl;
        cout << "  Min Price: " << result["min_price"] << endl;
        cout << "  Max Price: " << result["max_price"] << endl;

        // Print best bid and ask
        cout << "\nBest Bids/Asks:" << endl;
        cout << "  Best Bid Price: " << result["best_bid_price"] << endl;
        cout << "  Best Bid Amount: " << result["best_bid_amount"] << endl;
        cout << "  Best Ask Price: " << result["best_ask_price"] << endl;
        cout << "  Best Ask Amount: " << result["best_ask_amount"] << endl;

        // Print bids
        cout << "\nOrder Book (Bids):" << endl;
        if (result.contains("bids") && result["bids"].is_array()) {
            for (const auto& bid : result["bids"]) {
                cout << "  Price: " << bid[0] << ", Amount: " << bid[1] << endl;
            }
        }

        // Print asks (if any)
        cout << "\nOrder Book (Asks):" << endl;
        if (result.contains("asks") && result["asks"].is_array() && !result["asks"].empty()) {
            for (const auto& ask : result["asks"]) {
                cout << "  Price: " << ask[0] << ", Amount: " << ask[1] << endl;
            }
        } else {
            cout << "  No asks available." << endl;
        }

        // Print open interest
        cout << "\nOpen Interest: " << result["open_interest"] << endl;

    } catch (const exception& e) {
        cerr << "Error parsing or processing the JSON response: " << e.what() << endl;
    }
}

void modifyOrder(const string& password, const string& orderid, const string& amount, const string& price, const string& id) {
    json request = {
        {"jsonrpc" , "2.0"},
        {"id" , id},
        {"method" , "private/edit"},
        {"params" , {
            {"order_id", orderid},
            {"amount", amount},
            {"price", price}
        }}
    }; 

    const string url = "https://test.deribit.com/api/v2/private/edit";

    string response = curlRequest(url, request, password);

    try {
        // Parse the JSON response
        auto json_response = nlohmann::json::parse(response);
        putInJSONfile(json_response);

        if(json_response.contains("result")) {
            cout<<"Order modidifed successfuly, printing updated orders\n";
            getOpenOrders(password);
        }
        else if (json_response.contains("error"))
            cout<<"Error in modifying order, check inputted order ID\n";
    } catch (const exception& e) {
        cerr << "Error parsing or processing the JSON response: " << e.what() << endl;
    }


}

void getOpenOrders(const string& password, const string& id ) {
    json request = {
        {"jsonrpc" , "2.0"},
        {"id" , id},
        {"method" , "private/get_open_orders"},
        {"params" , {

        }}
    };

    const string url = "https://test.deribit.com/api/v2/private/get_open_orders";

    string response = curlRequest(url, request, password);

    try {
        // Parse the JSON response
        auto json_response = nlohmann::json::parse(response);
        putInJSONfile(json_response);

        // Extract the result array
        if (!json_response.contains("result") || !json_response["result"].is_array()) {
            throw runtime_error("Invalid or missing 'result' array in response");
        }

        auto orders = json_response["result"];

        // Print table header
        cout << left
             << setw(20) << "Instrument Name"
             << setw(20) << "Order ID"
             << setw(10) << "Price"
             << setw(15) << "Order State"
             << setw(10) << "Amount"
             << endl;
        cout << string(85, '-') << endl;

        // Iterate over the orders and print each one
        for (const auto& order : orders) {
            string instrument_name = order["instrument_name"];
            string order_id = order["order_id"];
            double filled_amount = order["filled_amount"];
            double price = order["price"];
            string order_state = order["order_state"];
            double amount = order["amount"];

            cout << left
                 << setw(20) << instrument_name
                 << setw(20) << order_id
                 << setw(10) << price
                 << setw(15) << order_state
                 << setw(10) << amount
                 << endl;
        }
    } catch (const exception& e) {
        cerr << "Error parsing or processing the JSON response: " << e.what() << endl;
    }
}

void getPosition(const string& password, const string& currency, const string& kind, const string& id) {
    const string& url = "https://test.deribit.com/api/v2/private/get_positions";
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "private/get_positions"},
        {"params", {{"currency", currency}, 
                    {"kind", kind}}},
    };

    string response = curlRequest(url, request, password);
    try {
        // Parse the JSON response
        auto json_response = nlohmann::json::parse(response);
        putInJSONfile(json_response);

        // Extract the "result" array
        if (!json_response.contains("result") || !json_response["result"].is_array()) {
            throw runtime_error("Invalid or missing 'result' array in response");
        }

        auto positions = json_response["result"];

        // Iterate over each position and print key information
        for (const auto& position : positions) {
            cout << "Position Data:" << endl;
            cout << "  Instrument Name: " << position["instrument_name"] << endl;
            cout << "  Kind: " << position["kind"] << endl;
            cout << "  Direction: " << position["direction"] << endl;

            cout << "\nPrices:" << endl;
            cout << "  Average Price: " << position["average_price"] << endl;
            cout << "  Mark Price: " << position["mark_price"] << endl;
            cout << "  Index Price: " << position["index_price"] << endl;
            cout << "  Settlement Price: " << position["settlement_price"] << endl;
            cout << "  Estimated Liquidation Price: " << position["estimated_liquidation_price"] << endl;

            cout << "\nMargins and Leverage:" << endl;
            cout << "  Initial Margin: " << position["initial_margin"] << endl;
            cout << "  Maintenance Margin: " << position["maintenance_margin"] << endl;
            cout << "  Open Orders Margin: " << position["open_orders_margin"] << endl;
            cout << "  Leverage: " << position["leverage"] << endl;

            cout << "\nProfits and Losses:" << endl;
            cout << "  Floating P/L: " << position["floating_profit_loss"] << endl;
            if (position.contains("realized_funding"))
                cout << "  Realized Funding: " << position["realized_funding"] << endl;
            cout << "  Realized P/L: " << position["realized_profit_loss"] << endl;
            cout << "  Total P/L: " << position["total_profit_loss"] << endl;

            cout << "\nSizes:" << endl;
            cout << "  Size (contracts): " << position["size"] << endl;
            cout << "  Size (currency): " << position["size_currency"] << endl;
            if (position.contains("interest_value"))
                cout << "  Interest Value: " << position["interest_value"] << endl;

            cout << "\n---------------------------------------\n";
        }
    } catch (const exception& e) {
        cerr << "Error parsing or processing the JSON response: " << e.what() << endl;
    }
}

void displayMenu() {
    cout << "\n=== Trading Application ===\n";
    cout << "1. Buy\n";
    cout << "2. Sell\n";
    cout << "3. Cancel Order\n";
    cout << "4. Modify Order\n";
    cout << "5. View Order Book\n";
    cout << "6. View Current Positions\n";
    cout << "7. View My Orders\n";
    cout << "8. Exit\n";
    cout << "Enter your choice: ";
}

vector<string> currencies = {
    "BTC", "ETH", "USDC", "USDT", "EURR", "any"
};

vector<string> kinds = {
    "future", "option", "spot", "future_combo", "option_combo"
};

int main() {


    // to give a uid to each query, the counter is incremented each time
    int id_counter = 1;

    // std::string accessToken = getAccessToken(clientId, clientSecret);

    // bool APIworking = getAPIResponse();  // Call the function to get the API response
    string accessToken = authenticate(Credentials::getClientId(), Credentials::getClientSecret());


    // if (!accessToken.empty()) {
    //     cout << "Authentication successful, access token: " << accessToken << endl;
    //     // buy(accessToken,"BTC-PERPETUAL","500000");
    //     // sell(accessToken, "BTC-PERPETUAL", "10", "100000");
    //     // getOrderBook("ETH-PERPETUAL");
    //     // getPosition(accessToken,"BTC", "future");

    // } else {
    //     cerr << "Authentication failed!" << endl;
    // }


    if (accessToken.empty()) {
        cerr << "Authentication failed! Exiting...\n";
        return 1;
    }

    cout << "Authentication successful!\n";

    int choice;
    do {
        displayMenu();
        cin >> choice;

        // Validate user input
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number between 1 and 7.\n";
            continue;
        }

        switch (choice) {
            case 1: {
                string symbol, price, quantity;
                cout << "Enter Instrument Name: ";
                cin >> symbol;
                cout << "Enter price: ";
                cin >> price;
                buy(accessToken, symbol, price, to_string(id_counter));
                id_counter++;
                break;
            }
            case 2: {
                string symbol, price, amount;
                cout << "Enter Instrument Name: ";
                cin >> symbol;
                cout << "Enter amount: ";
                cin >> amount;
                cout << "Enter price: ";
                cin >> price;
                sell(accessToken, symbol, amount, price, to_string(id_counter));
                id_counter++;
                break;
            }
            case 3: {
                string orderId;
                cout << "Enter Order ID to cancel: ";
                cin >> orderId;
                cancelOrder(accessToken, orderId, to_string(id_counter));
                id_counter++;
                break;
            }
            case 4: {
                string orderid, amount, price;
                cout << "Enter OrderID: ";
                cin >> orderid;
                cout << "Enter amount: ";
                cin >> amount;
                cout << "Enter price: ";
                cin >> price;
                modifyOrder(accessToken, orderid, amount, price, to_string(id_counter));
                id_counter++;
                break;
            }
            case 5: {
                string symbol;
                cout << "Enter instrument name to view order book: ";
                cin >> symbol;
                getOrderBook(symbol, to_string(id_counter));
                id_counter++;
                break;
            }            
            case 6: {
                string currency, kind;
                cout << "Enter currency to view the positions: ";
                currency = displayAndChoose(currencies, "currency");
                kind = displayAndChoose(kinds, "kind");
                
                getPosition(accessToken, currency, kind, to_string(id_counter));
                id_counter++;
                break;
            }    
            case 7: {
                getOpenOrders(accessToken, to_string(id_counter));
                id_counter++;
                break;
            }                      
            case 8:
                cout << "Exiting application. Goodbye!\n";
                break;
            default:
                cout << "Invalid choice. Please select a valid option.\n";
        }

    } while (choice != 8);

    return 0;
}   
