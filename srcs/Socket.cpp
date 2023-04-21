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

	//서버 종료후 바로 재시작시 bind()함수에서 중복 포트 에러 처리를 위한 소켓 세팅 로직
	int option = 1;
    if (setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        std::cout << "setsockopt error" << std::endl;
    }
	// TIME_WAIT 상태인 소켓 재사용 가능 (테스트가 끝난면 삭제 ㄱㄱ)

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
