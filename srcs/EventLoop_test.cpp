#include <iostream>

#include "EventLoop.hpp"

EventLoop::EventLoop(Config &con) {
    Socket sock(con);
    this->_kqFd = kqueue();
    this->_ChangeList = sock.GetChangeList();
    EventHandler();
}

void EventLoop::EventHandler() {
    int servcnt = _ChangeList.size(); // this->추가?
    struct kevent evntLst[8];
    struct kevent *curEvnts;
    int newEvnts;
    std::vector<std::pair<int, int> > check; ////
    while (1) {
        newEvnts = kevent(_kqFd, &_ChangeList[0], _ChangeList.size(), evntLst,
                          8, NULL);
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
                        std::pair<int, int> temp_pair; ////
                        temp_pair.first = clnt_fd;     ////
                        temp_pair.second = 0;          ////
                        check.push_back(temp_pair);    ////
                    } else
                        std::cout << "이상한 연결 요청" << std::endl;
                } else {
                    std::cout << "클라이언트(" << curEvnts->ident
                              << ")으로부터 request 들어옴" << std::endl;
                    HandleRequest(curEvnts);
                }
            } else if (curEvnts->filter == EVFILT_WRITE) {
                std::cout << "클라이언트(" << curEvnts->ident
                          << ")에게 응답을 보냄" << std::endl;
                /////////nhwang
                std::vector<std::pair<int, int> >::iterator it;
                it = check.begin();
                while (it != check.end()) {
                    if (it->first == (int)curEvnts->ident)
                        break;
                }
                if (it->second == 1) {
                    struct kevent tmpEvnt;
                    EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_WRITE, EV_DELETE,
                           0, 0, curEvnts->udata);
                    _ChangeList.push_back(tmpEvnt);
                } else {
                    MakeResponse(curEvnts);
                    it->second = 1;
                }
                /////////nhwang
            } else {
                std::cout << curEvnts->ident << "번 넌 뭐냐?" << std::endl;
            }
        }
    }
}

void EventLoop::HandleRequest(struct kevent *curEvnts) {
    // char buffer[3000] = {0};
    // int valread = read(curEvnts->ident, buffer, 3000);
    // if (valread == 0) {
    // 	std::cout << "fin 패킷 " << curEvnts->ident << "번 연결 종료"
    // 			  << std::endl;
    // 	close(curEvnts->ident);
    // 	return;
    // }
    // std::cout << "request msg :\n" << buffer << std::endl;
    // std::string request_str(buffer);
    // std::stringstream ss(request_str);

    ServerBlock *serverBlock = static_cast<ServerBlock *>(curEvnts->udata);
    Request *reque = ParseRequest(curEvnts->ident, this->_cli, *serverBlock);
    // Request reque(*serverBlock, ss);
    // (void)reque2;

    if (reque == NULL)
        return;
    std::string requestPath = reque->GetTarget();
    if (requestPath == "/")
        requestPath = serverBlock->GetRoot() + "/" + serverBlock->GetIndex()[0];
    // else
    //     requestPath = serverBlock->GetRoot() + requestPath;

    // if (requestPath == "/favicon.ico") {
    //     std::cout << "파비코리 컷!" << std::endl;
    //     delete (this->_cli[curEvnts->ident]);
    //     this->_cli.erase(curEvnts->ident);
    //     close(curEvnts->ident);
    //     return;
    // }

    std::cout << "request test ==============" << std::endl;
    std::cout << "Port : " << serverBlock->GetPort() << std::endl;
    std::cout << "ContentType : " << reque->GetContentType() << std::endl;
    std::cout << "Chunked : " << reque->GetChunked() << std::endl;
    std::cout << "ErrorCode : " << reque->GetErrorCode() << std::endl;
    std::cout << "ErrorMessages : " << reque->GetErrorMessages() << std::endl;
    std::cout << "Method : " << reque->GetMethod() << std::endl;
    std::cout << "Target : " << reque->GetTarget() << std::endl;

    struct kevent tmpEvnt;
    // EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_READ, EV_DELETE, 0, 0, 0);
    // _ChangeList.push_back(tmpEvnt);
    EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0,
           curEvnts->udata);
    _ChangeList.push_back(tmpEvnt);

    // std::cout << "Server : " << reque->GetServer() << std::endl;
    // std::cout << "" << reque->GetStream() << std::endl;

    // std::cout << "udata test ================" << std::endl;
    // std::cout << "root : " << serverBlock->GetRoot() << std::endl;
    // std::cout << "port : " << serverBlock->GetPort() << std::endl;
    // std::cout << "index : " << serverBlock->GetIndex()[0] << std::endl;
    // std::cout << "serverName : " << serverBlock->GetServerName()[0] <<
    // std::endl; std::cout << "location target: " <<
    // serverBlock->GetLocation()[0].GetLocationTarget() << std::endl; std::cout
    // << "location index: " << serverBlock->GetLocation()[0].GetIndex()[0] <<
    // std::endl; MakeResponse(curEvnts, requestPath);
}

void EventLoop::MakeResponse(struct kevent *curEvnts) {
    Request *reque = this->_cli[curEvnts->ident];
    ServerBlock *serverBlock = static_cast<ServerBlock *>(curEvnts->udata);
    std::string requestPath = reque->GetTarget();
    if (requestPath == "/")
        requestPath = serverBlock->GetRoot() + "/" + serverBlock->GetIndex()[0];
    else
        requestPath = serverBlock->GetRoot() + requestPath;

    std::cout << requestPath << std::endl;
    std::ifstream file(requestPath);
    if (!file.is_open()) {
        std::string response_str =
            "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        std::cout << response_str << std::endl;
        send(curEvnts->ident, response_str.c_str(), response_str.size(), 0);
    } else {
        std::ostringstream file_contents;
        file_contents << file.rdbuf();
        file.close();
        std::string response_body = file_contents.str() + "\r\n\r\n";
        std::string response_header = "HTTP/1.1 200 OK\r\nContent-Length: " +
                                      std::to_string(response_body.size()) +
                                      "\r\n\r\n";
        std::string response_str = response_header + response_body;
        int response_size = response_str.size();
        while (response_size > 0) {
            int res;
            res = send(curEvnts->ident, response_str.c_str(),
                       response_str.size(), 0);
            if (res == -1) {
                std::cout << "break" << std::endl;
                break;
            }
            std::cout << "Error: " << strerror(errno) << '\n';
            std::cout << "response size : " << response_size << std::endl;
            std::cout << "res : " << res << std::endl;
            response_size -= res; ///
        }
        std::cout << response_str.rfind("\r\n\r\n") << std::endl;
        std::cout << response_str.size() - 4 << std::endl;
        // if (response_str.rfind("\r\n\r\n") == response_str.size() - 4) {
        //     std::cout << "Event Delete" << std::endl;
        //     struct kevent tmpEvnt;
        //     EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_WRITE, EV_DELETE, 0, 0,
        //            curEvnts->udata);
        //     _ChangeList.push_back(tmpEvnt);
        // }
    }
    delete (this->_cli[curEvnts->ident]);
    this->_cli.erase(curEvnts->ident);
    // struct kevent tmpEvnt;
    // EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_WRITE, EV_DELETE, 0, 0,
    //        curEvnts->udata);
    //_ChangeList.push_back(tmpEvnt);
}

EventLoop::EventLoop() {}

EventLoop::~EventLoop() {}
