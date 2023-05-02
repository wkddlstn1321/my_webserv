#include <iostream>

#include "EventLoop.hpp"

EventLoop::EventLoop(Config &con) {
	Socket sock(con);
	this->_kqFd = kqueue();
	this->_ChangeList = sock.GetChangeList();
	// MakeHtmlFile(con);
	EventHandler();
}

void EventLoop::EventHandler() {
	int servcnt = this->_ChangeList.size(); // this->추가?
	struct kevent evntLst[16];
	struct kevent *curEvnts;
	int newEvnts;
	while (1) {
		newEvnts = kevent(_kqFd, &_ChangeList[0], _ChangeList.size(), evntLst,
						  16, NULL);
		_ChangeList.clear();
		for (int i = 0; i < newEvnts; i++) {
			std::cout
				<< "========================================================"
				<< std::endl;
			curEvnts = &evntLst[i];
			if (curEvnts->filter == EVFILT_READ) {
				if (curEvnts->ident < (uintptr_t)servcnt + 3) { // 서버 소켓?
					int clnt_fd;
					if (curEvnts->udata != NULL) {
						ServerBlock *sptr =
							static_cast<ServerBlock *>(curEvnts->udata);
						clnt_fd = accept(curEvnts->ident, 0, 0);
						std::cout << sptr->GetPort() << "에 새 클라이언트("
								  << clnt_fd << ") 연결" << std::endl;
						fcntl(clnt_fd, F_SETFL, O_NONBLOCK);
						struct kevent tmpEvnt;
						EV_SET(&tmpEvnt, clnt_fd, EVFILT_READ,
							   EV_ADD | EV_ENABLE, 0, 0, curEvnts->udata);
						_ChangeList.push_back(tmpEvnt);
					} else
						std::cout << "이상한 연결 요청" << std::endl;
				} else {
					std::cout << "클라이언트(" << curEvnts->ident
							  << ")으로부터 request 들어옴" << std::endl;
					HandleRequest(curEvnts);
				}
			} else if (curEvnts->filter == EVFILT_WRITE) {
				std::cout << "클라이언트(" << curEvnts->ident
						  << ")에게 Write 이벤트 발생" << std::endl;
				if (this->_response.find(curEvnts->ident) ==
					this->_response.end())
					MakeResponse(curEvnts);
				else
					SendResponse(curEvnts);
			} else {
				std::cout << curEvnts->ident << "번 알 수 없는 이벤트 필터("
						  << curEvnts->filter << ") 발생" << std::endl;
				close(curEvnts->ident);
				EraseMemberMap(curEvnts->ident);
			}
		}
	}
}

void EventLoop::HandleRequest(struct kevent *curEvnts) {
	ServerBlock *serverBlock = static_cast<ServerBlock *>(curEvnts->udata);
	Request *reque = ParseRequest(curEvnts->ident, this->_cli, *serverBlock);
	if (reque == NULL)
		return;
	struct kevent tmpEvnt;
	EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0,
		   curEvnts->udata);
	_ChangeList.push_back(tmpEvnt);
	/////////////////////////////////
	// char buf[3000];
	// while (true) {
	// 	int read_size = read(curEvnts->ident, buf, sizeof(buf));
	// 	if (read_size > 0) {
	// 		std::cout << read_size << "byte read keep going!" << std::endl;
	// 		buf[read_size] = '\0';
	// 		this->_request[curEvnts->ident] += buf;
	// 	}
	// 	if (read_size == -1) {
	// 		std::cout << "read failed" << std::endl;
	// 		close(curEvnts->ident);
	// 		EraseMemberMap(curEvnts->ident);
	// 		_request[curEvnts->ident] = "";
	// 		return;
	// 	} else {
	// 		if (_request[curEvnts->ident].size() == 0) {
	// 			if (close(curEvnts->ident) < 0) {
	// 				_request.erase(curEvnts->ident);
	// 				EraseMemberMap(curEvnts->ident);
	// 				std::cout << "close" << curEvnts->ident <<
	// "fail"
	// 						  << std::endl;
	// 			} else
	// 				std::cout << "close " << curEvnts->ident << "
	// success⭕️"
	// 						  << std::endl;
	// 			return;
	// 		}
	// 		std::stringstream ss(_request[curEvnts->ident]);
	// 		Request *request = new Request(*serverBlock, ss);
	// 		_cli.insert(std::pair<int, Request *>(curEvnts->ident,
	// request)); 		struct kevent tmpEvnt; 		EV_SET(&tmpEvnt,
	// curEvnts->ident, EVFILT_WRITE, EV_ADD | EV_ENABLE,
	// 0, 0, curEvnts->udata); 		_ChangeList.push_back(tmpEvnt);
	// return;
	// 	}
	// }
	/////////////////////////////////
}

void EventLoop::MakeResponse(struct kevent *curEvnts) {
	int error = 0;
	socklen_t error_len = sizeof(error);
	int ret =
		getsockopt(curEvnts->ident, SOL_SOCKET, SO_ERROR, &error, &error_len);
	if (ret == -1 || error != 0) {
		// Socket is dead, close it and return
		close(curEvnts->ident);
		EraseMemberMap(curEvnts->ident);
		std::cout << "이미 뒤진 소켓 MakeResponse 에서 발견되다" << std::endl;
		return;
	}
	std::cout << curEvnts->ident << " 에게 보낼 응답 msg 생성 " << std::endl;
	if (_cli.find(curEvnts->ident) == _cli.end()) {
		std::cout << "dlfjsruddnrk dlTdmfRk??" << std::endl;
		close(curEvnts->ident);
		return;
	}
	Request *reque = this->_cli[curEvnts->ident];
	ServerBlock *serverBlock = static_cast<ServerBlock *>(curEvnts->udata);
	std::string requestPath = reque->GetTarget();

	if (requestPath == "/")
		requestPath = serverBlock->GetLocation()[0].GetRoot() + "/" +
					  serverBlock->GetLocation()[0].GetIndex()[0];
	else
		requestPath = serverBlock->GetRoot() + requestPath;
	if (requestPath.find(".php") == requestPath.size() - 4) {
		PhStart(curEvnts);
		EraseMemberMap(curEvnts->ident);
		// close(curEvnts->ident);
		return;
	}
	std::string response_str;
	std::cout << requestPath << std::endl;
	if (_html.find(requestPath) == _html.end()) {
		std::ifstream file(requestPath);
		if (!file.is_open()) {
			response_str =
				"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
			send(curEvnts->ident, response_str.c_str(), response_str.size(), 0);
		} else {
			std::ostringstream file_contents;
			file_contents << file.rdbuf();
			file.close();

			std::string response_body = file_contents.str() + "\r\n\r\n";
			std::string response_header =
				"HTTP/1.1 200 OK\r\nContent-Length: " +
				std::to_string(response_body.size()) + "\r\n\r\n";
			response_str = response_header + response_body;
			_html[requestPath] = response_str;
		}
	}
	this->_response[curEvnts->ident] = _html[requestPath];
	// this->_response[curEvnts->ident] = response_str;
}

void EventLoop::SendResponse(struct kevent *curEvnts) {
	int error = 0;
	socklen_t error_len = sizeof(error);
	int ret =
		getsockopt(curEvnts->ident, SOL_SOCKET, SO_ERROR, &error, &error_len);
	if (ret == -1 || error != 0) {
		// Socket is dead, close it and return
		close(curEvnts->ident);
		EraseMemberMap(curEvnts->ident);
		std::cout << "이미 뒤진 소켓 SendResponse 에서 발견되다" << std::endl;
		return;
	}
	std::cout << curEvnts->ident << " 에게 응답 msg 전송 " << std::endl;
	std::string response_str = this->_response[curEvnts->ident];
	int response_size = response_str.size();
	int res = send(curEvnts->ident, response_str.c_str(), response_size,
				   MSG_NOSIGNAL);
	if (res > 0) {
		std::cout << curEvnts->ident << "번 다시 보내" << std::endl;
		this->_response[curEvnts->ident].erase(0, res);
		return;
	}
	if (res == -1) {
		std::cout << "Write fail" << '\n';
		EraseMemberMap(curEvnts->ident);
		// delete (this->_cli[curEvnts->ident]);
		// this->_cli.erase(curEvnts->ident);
		// this->_response.erase(curEvnts->ident);
		close(curEvnts->ident);
		return;
	} else {
		std::cout << "전송 완료 해쓰여 " << std::endl;
	}
	std::cout << "Event Delete" << std::endl;
	struct kevent tmpEvnt;
	EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_WRITE, EV_DELETE, 0, 0,
		   curEvnts->udata);
	_ChangeList.push_back(tmpEvnt);
	EraseMemberMap(curEvnts->ident);
}

// void EventLoop::SendResponse(struct kevent *curEvnts) {
// 	std::cout << curEvnts->ident << " 에게 응답 msg 전송 " << std::endl;
// 	std::string response_str = this->_response[curEvnts->ident];
// 	int response_size = response_str.size();
// 	int read_size = 0;
// 	while (response_size > 0) {
// 		int res;
// 		res = send(curEvnts->ident, response_str.c_str() + read_size,
// 				   response_size, MSG_NOSIGNAL);
// 		if (res == -1) {
// 			std::cout << "Error: " << strerror(errno) << '\n';
// 			std::cout << curEvnts->ident << "번 다시 보내" <<
// std::endl; 			this->_response[curEvnts->ident].erase(0,
// read_size); 			return; 		} else {
// std::cout << "response size : " << response_size << std::endl;
// std::cout << "res : " << res << std::endl;
// response_size -= res; 			read_size += res;
// 		}
// 		std::cout << "보낸 size : " << read_size << std::endl;
// 		std::cout << "남은 size : " << response_size << std::endl;
// 	}
// 	std::cout << "Event Delete" << std::endl;
// 	struct kevent tmpEvnt;
// 	EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_WRITE, EV_DELETE, 0, 0,
// 		   curEvnts->udata);
// 	_ChangeList.push_back(tmpEvnt);
// 	delete (this->_cli[curEvnts->ident]);
// 	this->_cli.erase(curEvnts->ident);
// 	this->_response.erase(curEvnts->ident);
// }

void EventLoop::PhStart(struct kevent *curEvnts) {
	int _pipe[2];
	int pid;
	// char buf[1024];

	pipe(_pipe);
	std::string msg = _cli[curEvnts->ident]->GetRequestMessages() + "\r\n\r\n";
	write(_pipe[1], msg.c_str(), sizeof(msg.c_str()));
	pid = fork(); // dup2를 할 경우엔 부모단에서 _pipe[1]로 리퀘스트 전체를
				  // 넘겨야함.
	/// 오히려 파싱하기 전에 전체 문장을 받아오는 부분에서 파싱전에 이 부분을
	/// 실행하는 게 합리적일 것 같음.
	//
	char req_method[] = "REQUEST_METHOD=POST";
	char query[] = "QUERY_STRING=Name=test&Age=123";
	char content_length[] = "CONTENT_LENGTH=17"; // 반드시 test / 123
	char content_type[] = "CONTENT_TYPE=application/x-www-form-urlencoded";
	char script_filename[] =
		"SCRIPT_FILENAME=/Users/jang-insu/webservTest/html";
	// char script_filename[] = "SCRIPT_FILENAME=/html/index.php";
	char script_name[] = "SCRIPT_NAME=../html";
	char server_protocol[] = "SERVER_PROTOCOL=HTTP/1.1";
	char redirect_status[] = "REDIRECT_STATUS=200";
	char document_root[] =
		"DOCUMENT_ROOT=/Users/jang-insu/webservTest/test_yong";
	char gateway_interface[] = "GATEWAY_INTERFACE=CGI/1.1";
	char request_uri[] = "REQUEST_URI=/index.php";
	char *_env[] = {req_method,      query,         content_length,
					script_filename, script_name,   server_protocol,
					redirect_status, document_root, gateway_interface,
					content_type,    request_uri,   NULL};
	if (pid == 0) // child
	{
		close(_pipe[1]);
		dup2(_pipe[0], 0);
		dup2(curEvnts->ident, 1);
		close(_pipe[0]);
		chdir("/Users/jang-insu/webservTest/html");
		// char str[] = "/opt/homebrew/bin/php-cgi";
		char str[] = "/opt/homebrew/bin/php";
		char str2[] = "index.php";

		char *arg[] = {str, str2, NULL};
		// std::cout << "=======in child!!!!!======" << std::endl;
		if (execve(arg[0], arg, _env) == -1) {
			std::cout << "exe err" << std::endl;
			exit(-1);
		}  // NULL??
	} else // Parant
	{
		// write(_pipe[1], msg.c_str(), sizeof(msg.c_str()));
		waitpid(pid, 0, 0);
		std::cout << "-------in P" << std::endl;
		printf("%s", msg.c_str());
		std::cout << "-------in P" << std::endl;
		std::cout << "-------in P3333" << std::endl;

		close(_pipe[1]);
		close(_pipe[0]);
	}
	// send(curEvnts->ident, "", 0, 0);
	// _cli[curEvnts->ident]->GetMethod curEvnts;
}

void EventLoop::EraseMemberMap(int key) {
	delete (this->_cli[key]);
	this->_cli.erase(key);
	this->_response.erase(key);
}

EventLoop::EventLoop() {}

EventLoop::~EventLoop() {}
