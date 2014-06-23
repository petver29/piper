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

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

namespace Redis {
class Decoder
{
 public:
  Decoder(const char* buf) : buf_(buf) {}
  int read_int() {
  	int len = 0;
    while(*buf_ != '\r') {
      len = (len*10)+(*buf_ - '0');
      ++buf_;
    }
    ++buf_;
    return len;
  }
  const char* buf_;
  int error_;
}; // Decoder
class Encoder {
 public:
  Encoder() = default;
  void lrange(std::string& channel, int64_t from, int64_t to) {

  }
  void encode(const std::string& buf) {
  	buf_.push_back('$');
  	write_int(buf.size());
  	buf_.push_back('\r');
  	buf_.push_back('\n');
  	buf_.insert(buf_.end(), std::begin(buf), std::end(buf));
  	buf_.push_back('\r');
  	buf_.push_back('\n');
  }
  void encode(int64_t) {
  	size_t len = buf_.size();
  	
  }
  std::string str() const {
  	return std::string(buf_.data(), buf_.size());
  }
  int write_digits(int64_t num) {
  	int len = 0;
  	while (num) {
  		buf_.push_back((num % 10) + '0');
  		num /= 10;
  	}
  	if (len) {
  	  std::reverse(buf_.end() - len, buf_.end());
  	  return len;
  	} else {
  	  buf_.push_back('0');
  	  return 1;
  	}
  }
  int write_int(int64_t num) {
  	int len = 0;
  	if (num < 0) {buf_.push_back('-'); ++len;}
  	return len + write_digits(std::abs(num));
  }
  std::vector<char> buf_;
}; // Command
}  // Redis

int main(int argc, char** argv) {
  using namespace std;
  const char* buf = "345\r\n";
  Redis::Decoder decoder(buf);
  int num = decoder.read_int();
  cout << num << endl;

  Redis::Encoder encoder;
  encoder.encode("LRANGE");
  cout << encoder.str() << endl;
}