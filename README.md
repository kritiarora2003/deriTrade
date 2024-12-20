# Deribit API Client in C++

This guide provides a step-by-step process for setting up a Deribit API client using C++ for both HTTP and WebSocket communication. The setup will include handling authentication, API requests, and connecting to WebSocket for real-time market data.

## Libraries Used

We will need the following libraries to interact with the Deribit API:

- **libcurl**: For making HTTP requests to the API endpoints.
- **nlohmann-json**: For JSON parsing and handling.
- **Boost**: For networking, especially WebSocket connections.
- **OpenSSL**: For SSL/TLS support in WebSocket connections.

To install the required libraries, use the following commands:

### Installing Required Libraries

```bash
sudo apt-get install libcurl4-openssl-dev
sudo apt-get install nlohmann-json3-dev
sudo apt install libboost-all-dev
sudo apt install libssl-dev
```

### Setting Up Your API Credentials

For security purposes, it’s important not to hard-code your API credentials in the source code. You can store your credentials in environment variables. 

1. **Edit `.bashrc`**:

```bash
nano ~/.bashrc
```

2. **Add Your Credentials** at the end of the file:

```bash
export CLIENT_ID="your_client_id_here"
export CLIENT_SECRET="your_client_secret_here"
```

3. **Apply the changes**:

```bash
source ~/.bashrc
```

### Testing the API Endpoint

To test the connectivity and ensure that the API works, use this endpoint to fetch the current server time:

```cpp
const char* url = "https://test.deribit.com/api/v2/public/get_time";
```

This should work successfully if your setup is correct.

### Building and Running the C++ Application

Once the dependencies are installed and the credentials are set, you can compile the C++ application with the following command:

```bash
g++ -std=c++11 -o app app.cpp -lcurl
./app
```

Or just use the Makefile, which will compile and run the code for you
```bash
make
```

## WebSocket Connection to Deribit API

Deribit provides a WebSocket API for real-time market data. We will use the [WebSocket++](https://github.com/zaphoyd/websocketpp) library to connect to Deribit's WebSocket server.

The code for connecting to a websocket is taken from (https://medium.com/nerd-for-tech/your-first-c-websocket-client-4e7b36353d26)

### Steps to Set Up WebSocket Connection

1. **Clone the WebSocket++ Library**:

```bash
git clone https://github.com/zaphoyd/websocketpp.git
cd websocketpp
```

2. **Prepare WebSocket Code**:

Save the `test.cpp` and `credentials.h` files inside the `websocketpp` folder. These files will handle WebSocket communication.

3. **Compile WebSocket Code**:

Once you have placed the necessary files in the `websocketpp` folder, compile the WebSocket code with the following command:

```bash
g++ -o test test.cpp -lssl -lcrypto -pthread
```

### WebSocket Server URL

The WebSocket server URL for Deribit’s test environment is:

```
wss://test.deribit.com/ws/api/v2
```

### References

- **Official Deribit API Documentation**:
  - [Deribit API v2.1.1 Documentation](https://docs.deribit.com/?python#deribit-api-v2-1-1)
  
- **Tutorials**:
  - [How to Build Your First C++ REST API Client](https://tradermade.com/tutorials/how-to-build-your-first-cpp-rest-api-client)
  - [Deribit API Guide](https://paperswithbacktest.com/wiki/deribit-api-guide)
