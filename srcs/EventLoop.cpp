#include <iostream>

#include "EventLoop.hpp"

EventLoop::EventLoop(Config &con) {
    Socket sock(con);
    this->_kqFd = kqueue();
    this->_ChangeList = sock.GetChangeList();
    EventHandler();
}

void EventLoop::EventHandler() {
    int servcnt = this->_ChangeList.size(); // this->추가?
    struct kevent evntLst[8];
    struct kevent *curEvnts;
    int newEvnts;
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
                std::cout << curEvnts->ident << "번 넌 뭐냐!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!?" << std::endl;
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
}

void EventLoop::MakeResponse(struct kevent *curEvnts) {
    std::cout << curEvnts->ident << " 에게 보낼 응답 msg 생성 " << std::endl;
    Request *reque = this->_cli[curEvnts->ident];
    ServerBlock *serverBlock = static_cast<ServerBlock *>(curEvnts->udata);
    std::string requestPath = reque->GetTarget();
    if (requestPath == "/" )
        requestPath = serverBlock->GetLocation()[0].GetRoot() + "/" +
                      serverBlock->GetLocation()[0].GetIndex()[0];
    else
        requestPath = serverBlock->GetRoot() + requestPath;
	
	std::cout << "===================reque=======================" << std::endl;
	std::cout << reque->GetContentType() << std::endl;
	std::cout << reque->GetChunked() << std::endl;
	std::cout << reque->GetErrorCode() << std::endl;
	std::cout << reque->GetMethod() << std::endl;
	std::cout << reque->GetServer().GetServerName()[0] << std::endl;


    std::string response_str;
    std::cout << requestPath << std::endl;
    std::ifstream file(requestPath);
    if (!file.is_open()) {
        response_str = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(curEvnts->ident, response_str.c_str(), response_str.size(), 0);
    } else {
        std::ostringstream file_contents;
        file_contents << file.rdbuf();
        file.close();

        std::string response_body = file_contents.str() + "\r\n\r\n";
        // std::string response_header =
            // "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream; "
            // "charset=UTF-8\r\nContent-Length: 765\r\nConnection: close\r\n" +
            // std::to_string(response_body.size()) + "\r\n\r\n";
        std::string response_header = "HTTP/1.1 200 OK\r\nContent-Length: " +
                                      std::to_string(response_body.size()) +
                                      "\r\n\r\n";
        response_str = response_header + response_body;
    }
    this->_response[curEvnts->ident] = response_str;
}

void EventLoop::SendResponse(struct kevent *curEvnts) {
    std::cout << curEvnts->ident << " 에게 응답 msg 전송 " << std::endl;
    std::string response_str = this->_response[curEvnts->ident];
    int response_size = response_str.size();
    int read_size = 0;
    ////////////////////////////////////////////////////////////////////////////////////////
    // int total_bytes_sent = 0;
    // int remaining_bytes =
    //     response_str.size(); // data_size는 전송할 데이터의 크기

    // while (remaining_bytes > 0) {
    //     int bytes_sent =
    //         send(curEvnts->ident, response_str.c_str() + total_bytes_sent,
    //              remaining_bytes, MSG_NOSIGNAL);

    //     if (bytes_sent == -1) {
    //         // if (errno == EAGAIN || errno == EWOULDBLOCK) {
    //             // 출력 버퍼가 가득 차서 전송할 수 없는 경우
    //             continue;
    //         // } else {
    //         //     // 다른 에러가 발생한 경우
    //         //     perror("send() failed");
    //         //     break;
    //         // }
    //     } else if (bytes_sent == 0) {
    //         // 연결이 끊어진 경우
    //         std::cout << "dis connect 발생 " << std::endl;
    //         break;
    //     }

    //     total_bytes_sent += bytes_sent;
    //     remaining_bytes -= bytes_sent;
    // }
    // if (remaining_bytes > 0) {
    //     std::cout << "noooooooooooo" << std::endl;
    //     return;
    //     // 전송되지 않은 데이터가 있는 경우, 다시 보내기 처리
    // }
    /////////////////////////////////////////////////////////////////////////////////
    while (response_size > 0) {
        int res;
        res = send(curEvnts->ident, response_str.c_str() + read_size,
                   response_size, MSG_NOSIGNAL);
        if (res == -1) {
            std::cout << "Error: " << strerror(errno) << '\n';
            std::cout << curEvnts->ident << "번 다시 보내" << std::endl;
            this->_response[curEvnts->ident].erase(0, read_size);
            return;
        } else {
            std::cout << "response size : " << response_size << std::endl;
            std::cout << "res : " << res << std::endl;
            response_size -= res;
            read_size += res;
        }
        std::cout << "보낸 size : " << read_size << std::endl;
        std::cout << "남은 size : " << response_size << std::endl;
        // std::cout << response_str.size() - 4 << std::endl;
    }
    /////////////////////////////
    std::cout << "Event Delete" << std::endl;
    struct kevent tmpEvnt;
    EV_SET(&tmpEvnt, curEvnts->ident, EVFILT_WRITE, EV_DELETE, 0, 0,
           curEvnts->udata);
    _ChangeList.push_back(tmpEvnt);
    delete (this->_cli[curEvnts->ident]);
    this->_cli.erase(curEvnts->ident);
    this->_response.erase(curEvnts->ident);
}

EventLoop::EventLoop() {}

EventLoop::~EventLoop() {}
