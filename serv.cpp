// Copyright (c) 2014 Maxim Trokhimtchouk
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "iorequests.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>
#include <list>
#include <string>
#include <functional>
#include <algorithm>
#include <signal.h>

#define unless(cond) if(!(cond))
#define BREAK raise(SIGTRAP)

const int PIPER_CONN_BACKLOG = 10;
const size_t PIPER_CONN_BUFSIZE = 32;
const size_t PIPER_SEND_MAXSIZE = 255;
const size_t PIPER_READ_MAXSIZE = 255;

void SysErrorExit(const std::string& msg = "") {
  BREAK;
  perror(msg.c_str());
  exit(1);
}

class Session {
 public:
  Session(Piper::IORequests& ioreqs, int socket)
  : ioreqs_(ioreqs), socket_(socket) {
    ioreqs_.read(socket_, std::bind(&Session::read, this));
    ioreqs_.error(socket_, std::bind(&Session::error, this));
  }
  void read() {
    buf_.resize(kMaxSize);
    int msgsize = recv(socket_, &buf_[0], kMaxSize, 0);
    if (msgsize < 0) SysErrorExit();
    buf_.resize(msgsize);
    if (msgsize > 0)
      ioreqs_.write(socket_, std::bind(&Session::write, this));
  }
  void write() {
    int msgsize = send(socket_, &buf_[0], buf_.size(), 0);
    if (msgsize < 0) SysErrorExit();
    buf_.erase(buf_.begin(), buf_.begin() + msgsize);
    if (buf_.empty())
      ioreqs_.read(socket_, std::bind(&Session::read, this));
    else
      ioreqs_.write(socket_, std::bind(&Session::write, this));
  }
  void error() {
  }
 private:
  const int kMaxSize = 256;
  std::vector<char> buf_;
  Piper::IORequests& ioreqs_;
  int socket_;
};

class Server {
 public:
  Server(Piper::IORequests& ioreqs)
  : ioreqs_(ioreqs) {
    socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ < 0) SysErrorExit();
    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(1234);
    if (bind(socket_, (sockaddr*)&address, sizeof(address))) SysErrorExit();
    // TODO: BACKLOG need to be an option at some point
    if (listen(socket_, PIPER_CONN_BACKLOG)) SysErrorExit();
    ioreqs_.read(socket_, std::bind(&Server::accept_client, this));
    ioreqs_.error(socket_, std::bind(&Server::error, this));
  }
  void accept_client() {
    sockaddr_in clientaddr;
    socklen_t addrsize = sizeof(clientaddr);
    int clientsock = accept(socket_, (sockaddr*)&clientaddr, &addrsize);
    // TODO(mtrokhim): do not crush on errors
    if (clientsock <= 0) SysErrorExit();
    sessions_.push_back(new Session(ioreqs_, clientsock));
    ioreqs_.read(socket_, std::bind(&Server::accept_client, this));
  }
  void error() {
    SysErrorExit();
  }
 private:
  std::list<Session*> sessions_;
  Piper::IORequests& ioreqs_;
  int socket_;
};

int main(int argc, char** argv) {
  Piper::IORequests ioreqs;
  Server server(ioreqs);
  for (;;) {if (ioreqs.process_one()) SysErrorExit();}
  return 0;
}
