#include <iostream>

#include "Socket.hpp"

Socket::Socket(Config& con)
{
	Config::iterator it ,its;
	it = con._ServerBlockObject.begin();
	its = con._ServerBlockObject.end();
	for ( ; it != its ; it++) {
		ServerInit(it.base()->GetPort());
	}
}

Socket::~Socket() {}

void Socket::ServerInit(int port)
{
	int serv_sock;
	socklen_t clnt_size;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	listen(serv_sock, 5);
}
