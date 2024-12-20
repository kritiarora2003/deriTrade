#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <string>
#include <cstdlib>  // For getenv
#include <iostream> // For logging (optional)

class Credentials {
public:
    static std::string getClientId() {
        const char* clientId = std::getenv("CLIENT_ID");
        if (clientId == nullptr) {
            std::cerr << "Warning: CLIENT_ID environment variable not set. Using default value." << std::endl;
            return "DefaultClientId";  // Default value
        }
        std::cout<<clientId<<std::endl;
        return std::string(clientId);
    }

    static std::string getClientSecret() {
        const char* clientSecret = std::getenv("CLIENT_SECRET");
        if (clientSecret == nullptr) {
            std::cerr << "Warning: CLIENT_SECRET environment variable not set. Using default value." << std::endl;
            return "DefaultClientSecret";  // Default value
        }
        return std::string(clientSecret);
    }
};

#endif
