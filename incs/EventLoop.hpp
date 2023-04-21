#ifndef EVENTLOOP_HPP
# define EVENTLOOP_HPP

#include"Config.hpp"
#include"Socket.hpp"
#include"Request.hpp"
#include<fcntl.h>
#include<map>


class EventLoop
{
private:
	int	_kqFd;
	std::vector<struct kevent> _ChangeList;
	std::map<int, Request *> _cli;
	std::map<int, std::string> _response;
	EventLoop();

public:
	EventLoop(Config& con);
	~EventLoop();
	void	EventHandler();
	void	HandleRequest(struct kevent *curEvnts);
	void	MakeResponse(struct kevent *curEvnts);
	void	SendResponse(struct kevent *curEvnts);
};

#endif
