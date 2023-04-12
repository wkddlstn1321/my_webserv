#ifndef SOCKET_HPP
# define SOCKET_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include"Config.hpp"

class Socket
{
private:
	/* data */

public:
	Socket(Config& con);
	~Socket();
	void ServerInit(int port);
};

#endif
