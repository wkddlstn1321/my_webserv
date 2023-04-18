#include <iostream>

#include "Socket.hpp"

Socket::Socket(Config& con)
{
	Config::iterator it ,its;
	it = con._ServerBlockObject.begin();
	its = con._ServerBlockObject.end();
	for ( ; it != its ; it++) {
		ServerInit(it);
	}
}

Socket::~Socket() {}

void Socket::ServerInit(Config::iterator it)
{
	int servSock;
	struct sockaddr_in serv_addr;

	servSock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(it->GetPort());
	if (bind(servSock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		if (errno == EADDRINUSE)
			std::cout << "duplicate bind err" << std::endl;
		else
			std::cout << "bind err" << std::endl;
	}
	if (listen(servSock, 5) < 0)
		std::cout << "listen err" << std::endl;
	AddSocketEvent(servSock, *it);
}


//changelist 에 서버 소켓에 대한 이벤트 정보 저장
void Socket::AddSocketEvent(int servSock, ServerBlock& ServerBlock)
{
	struct kevent tmpEvnt;
	void *ptr = static_cast<void *>(&ServerBlock);
	EV_SET(&tmpEvnt, servSock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, ptr);
	this->_changList.push_back(tmpEvnt);
}

std::vector<struct kevent>& Socket::GetChangeList()
{
	return (this->_changList);
}
