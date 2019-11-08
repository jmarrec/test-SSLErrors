#if (defined (__GNUC__))
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
#include <cpprest/http_client.h>
#if (defined (__GNUC__))
  #pragma GCC diagnostic pop
#endif

#include <iostream>
#include <sstream>
#include <regex>

int main(int argc, char * argv[]) {

  // If I change this to http, it works
  auto ip = web::http::client::http_client(U("https://checkip.amazonaws.com/"))
    .request(web::http::methods::GET)
    .then([](web::http::http_response response) {
      auto statusCode = response.status_code();
      std::cout << statusCode << std::endl;
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
