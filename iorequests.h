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

#ifndef   __PIPER_IOREQUESTS_H__
#define   __PIPER_IOREQUESTS_H__

#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <list>
#include <functional>
#include <algorithm>

namespace Piper {
class IORequests {
 public:
   typedef std::function<void()> Handler;
   typedef std::pair<int, Handler> Request;
   typedef std::list<Request> Requests;
   void read(int fd, Handler handler) {readreq_.push_back({fd, handler});}
   void write(int fd, Handler handler) {writereq_.push_back({fd, handler});}
   void error(int fd, Handler handler) {errorreq_.push_back({fd, handler});}
   int process_one() {
     fd_set readfds;
     fd_set writefds;
     fd_set errorfds;
     int maxfd = 0;
     maxfd = setup_requests(readfds, readreq_, maxfd);
     maxfd = setup_requests(writefds, writereq_, maxfd);
     maxfd = setup_requests(errorfds, errorreq_, maxfd);
     if(select(++maxfd, &readfds, &writefds, &errorfds, NULL) < 0) return errno;
     process_requests(readfds, readreq_);
     process_requests(writefds, writereq_);
     process_requests(writefds, errorreq_);
     return 0;
   }
 private:
  int setup_requests(fd_set& fds, Requests& reqs, int maxfd) {
    FD_ZERO(&fds);
    for(auto& req: reqs) {
      maxfd = std::max(maxfd, req.first);
      FD_SET(req.first, &fds);
    }
    return maxfd;
  }
  void process_requests(fd_set& fds, Requests& reqs) {
    for(auto req = reqs.begin(); req != reqs.end(); ++req) {
      if (FD_ISSET(req->first, &fds)) {
        auto cur = req; ++req;
        FD_CLR(cur->first, &fds);
        cur->second();
        reqs.erase(cur);
      }
    }
  }
  std::list<Request> readreq_;
  std::list<Request> writereq_;
  std::list<Request> errorreq_;
}; // IORequests
}  // Piper

#endif // __PIPER_IOREQUESTS_H__
