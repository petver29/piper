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

class Client {
 public:
  Client(Piper::IORequests& ioreqs) : ioreqs_(ioreqs)
  {
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) SysErrorExit();
    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(1234);
  
    if (inet_aton("localhost", &address.sin_addr) <= 0)
      SysErrorExit();

    if (connect(socket_, (sockaddr*)&address, sizeof(address)) < 0)
      SysErrorExit();

    ioreqs_.error(socket_, std::bind(&Client::error, this));
    read_line();
  }
  void read_line() {
    std::cin >> buf_;
    ioreqs_.write(socket_, std::bind(&Client::write, this));
  }
  void write() {
    int msgsize = send(socket_, &buf_[0], buf_.size(), 0);
    if (msgsize < 0) SysErrorExit();
    ioreqs_.read(socket_, [this](){
      buf_.resize(kMaxSize);
      int msgsize = recv(socket_, &buf_[0], kMaxSize, 0);
      if (msgsize < 0) SysErrorExit();
      buf_.resize(msgsize);
      std::cout << buf_ << std::endl;
      read_line();
    });
  }
  void error() {
    SysErrorExit();
  }
 private:
  const int kMaxSize = 256;
  Piper::IORequests& ioreqs_;
  std::string buf_;
  int socket_;
};

int main(int argc, char** argv) {
  Piper::IORequests ioreqs;
  Client client(ioreqs);
  for (;;) {if (ioreqs.process_one()) SysErrorExit();}
  return 0;
}
