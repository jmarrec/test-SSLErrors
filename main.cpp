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

///@brief Helper class that prints the current certificate's subject
///       name and the verification results.
template <typename Verifier>
class verbose_verification
{
public:
  verbose_verification(Verifier verifier)
    : verifier_(verifier)
  {}

  bool operator()(
    bool preverified,
    boost::asio::ssl::verify_context& ctx
  )
  {
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    bool verified = verifier_(preverified, ctx);
    std::cout << "Verifying: " << subject_name << "\n"
                 "Verified: " << verified << std::endl;
    return verified;
  }
private:
  Verifier verifier_;
};

///@brief Auxiliary function to make verbose_verification objects.
template <typename Verifier>
verbose_verification<Verifier>
make_verbose_verification(Verifier verifier)
{
  return verbose_verification<Verifier>(verifier);
}

int main(int argc, char * argv[]) {

  web::http::client::http_client_config config;
  config.set_validate_certificates(true);
  config.set_ssl_context_callback([](boost::asio::ssl::context& ctx)
                                    {
                                        // this code is invoked right after calling client.request
                                        ctx.set_verify_mode(boost::asio::ssl::context::verify_peer);
                                        // ctx.set_verify_callback(make_verbose_verification(boost::asio::ssl::rfc2818_verification("bcl.nrel.gov")));
                                        ctx.set_verify_callback(boost::asio::ssl::rfc2818_verification("*.nrel.gov"));
                                    });

  // Note: If I change this to http, it works without setting validate_certificates(false)
  // std::string remote = "https://checkip.amazonaws.com/";

  std::string remote("https://bcl.nrel.gov/");
  auto ip = web::http::client::http_client(remote, config)
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
