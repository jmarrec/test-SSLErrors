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

  web::http::client::http_client_config config;
  config.set_validate_certificates(false);

  // Note: If I change this to http, it works without setting validate_certificates(false)
  auto ip = web::http::client::http_client(U("https://checkip.amazonaws.com/"), config)
  .request(web::http::methods::GET)
  .then([](web::http::http_response response) {
      auto statusCode = response.status_code();
      if (statusCode != 200) {
        std::stringstream ss;
        ss << "Error: response code was " << statusCode;
        std::cout << ss.str() << '\n';
        return ss.str();
      }

      auto body = response.extract_utf8string(true).get();

      // Remove trailing line ending
      return body.erase(body.find_last_not_of("\n") + 1);
    })
  .get();

  std::regex ipRegex("^\\d{1,3}(?:\\.\\d{1,3}){3}$");
  std::smatch ipMatch;
  bool isOnline = std::regex_search(ip, ipMatch, ipRegex);
  std::cout << "isOnline = " << std::boolalpha << isOnline << '\n';
  if (isOnline) {
    std::cout << "IP=" << ipMatch[0] << '\n';
  }

  return 0;
}
