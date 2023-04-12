#include "Config.hpp"
#include "Socket.hpp"
#include <iostream>
#include <string>

int main(int ac, char **av) {
    if (ac != 2) {
        std::cout << "Usage ./webserv [Configuration]" << std::endl;
        exit(1);
    }
    Config con(static_cast<std::string>(av[1]));
    Socket sock(con);
}
