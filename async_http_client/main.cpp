#include <iostream>
#include "http/http.hpp"

int main()
{	
	boost::asio::io_context context;
	net::async_http_client client(context, "api.openweathermap.org");
	client.build_request(net::request_method::get, "/data/2.5/weather?q=Baku&appid=0824a96cd05f382c76cbbd3e1bbdc932");
	client.execute();

	auto response = client.consume_response();
	std::cout << boost::beast::buffers_to_string(response.value().body().data());

	return 0;
}
