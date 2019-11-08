#include <cpprest/http_client.h>
#include <iostream>
#include <sstream>
#include <regex>

int main(int argc, char * argv[]) {

  auto ip = web::http::client::http_client(U("https://checkip.amazonaws.com/"))
    .request(web::http::methods::GET)
    .then([](web::http::http_response response) {
      auto statusCode = response.status_code();
      if (statusCode != 200) {
        std::stringstream ss;
        ss << "Error: response code was " << statusCode;
        return ss.str();
      }

      auto body = response.extract_utf8string(true).get();
      // Remove trailing line ending
      return body.erase(body.find_last_not_of("\n") + 1);
    })
    .get();

  std::regex ipRegex("^\\d{1,3}(?:\\.\\d{1,3}){3}$");
  std::cout << "isOnline = " << std::boolalpha << std::regex_search(ip, ipRegex);

  return 0;

}
