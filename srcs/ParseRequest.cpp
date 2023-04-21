#include "Request.hpp"
#include "StateCode.hpp"
#include "get_next_line.h"
#include "utils.hpp"
#include <algorithm>
#include <map>

Request *ParseRequest(int fd, std::map<int, Request *> &clients,
                      ServerBlock &server) {
    // 소켓 통신 관련 파일 입출력 함수는 오프셋관리를 해주지 않기 때문에 gnl로
    // read를 하는게 정상 동작하지 않을 수 도 있다.
    char *line = get_next_line(fd);
    std::stringstream ss;
    
    while (line) {
        ss << line;
        free(line);
        line = get_next_line(fd);
    }

    if (ss.str() == "") {
        if (close(fd) < 0)
            std::cout << "close" << fd << "fail" << std::endl;
        else
            std::cout << "close " << fd << " success⭕️" << std::endl;
        return NULL;
    }
    std::cout << ss.str() << std::endl;

    std::map<int, Request *>::iterator clientsIterator = clients.find(fd);
    if (clientsIterator == clients.end()) {
        Request *request = new Request(server, ss);
        clients.insert(std::pair<int, Request *>(fd, request));
    } else {
        Request *request = clients[fd];
        std::vector<std::string> body = Split2(ss.str(), CRLF);
        try {
            request->SetBody(body.begin());
        } catch (const Request::BodySizeError &e) {
            request->SetErrorCode(PayloadTooLarge);
            request->SetErrorMessages(e.what());
        } catch (const std::exception &e) {
            request->SetErrorCode(BadRequest);
            request->SetErrorMessages(e.what());
        }
    }
    return clients[fd];
}
