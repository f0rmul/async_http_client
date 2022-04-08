#include "http.hpp"

namespace net
{	/* <async_http_client> */

	void async_http_client::execute()
	{	
		auto callable = [this](const std::string_view port)  // prepairing for passing to the new thread
		{
			asio::co_spawn(m_context.get_executor(), establish_connection(m_host, port), asio::detached);
			m_context.run();
		};
		detail::spawn_thread(callable, "80");// default http port
	}

	asio::awaitable<void> async_http_client::establish_connection(const std::string_view host, const std::string_view port)
	{
		asio::ip::tcp::resolver resolver(asio::make_strand(m_context));

		system::error_code ec;
		const auto resolved_address = co_await resolver.async_resolve(host, port, asio::redirect_error(asio::use_awaitable,ec));

		if (ec)	
			co_return;
	
		co_await asio::async_connect(m_socket, resolved_address, asio::redirect_error(asio::use_awaitable, ec));

		if (ec)
			co_return;

		co_await perform_request(m_options.method, m_options.target);
	}

	asio::awaitable<void> async_http_client::perform_request(request_method method, const std::string_view target)
	{
		using namespace boost::beast;
		system::error_code ec;

		request_t request;
		request.version(11); // http version
		request.method(method);
		request.target(target.data());

		request.set(http::field::host, m_host.data());
		request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		co_await http::async_write(m_socket, request, asio::redirect_error(asio::use_awaitable, ec));

		if (ec)
			co_return;

		co_await produce_response();
	}

	asio::awaitable<void> async_http_client::produce_response()
	{
		using namespace boost::beast;
		system::error_code ec;
		flat_buffer buffer;

		co_await http::async_read(m_socket, buffer, m_response, asio::redirect_error(asio::use_awaitable, ec));

		if (ec)
			co_return;

		is_ready_response.store(true,std::memory_order::release);
		m_cv.notify_all();
	}

	std::optional<async_http_client::response_t> async_http_client::consume_response(std::chrono::milliseconds awaiting_time)
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		if (m_cv.wait_for(lk, awaiting_time, [this] {return is_ready_response.load(std::memory_order::acquire);}))
			return { m_response };

		return std::nullopt;
	}

	void async_http_client::build_request(request_method method, const std::string_view target) noexcept
	{
		m_options.method = method;
		m_options.target = target;
	}
	/* </async_http client>*/
} // net
