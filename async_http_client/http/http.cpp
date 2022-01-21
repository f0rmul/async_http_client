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

		const auto resolved_address = co_await resolver.async_resolve(host, port, asio::use_awaitable);

		co_await asio::async_connect(m_socket, resolved_address, asio::use_awaitable);

		co_await asio::co_spawn(m_context.get_executor(), perform_request(m_options.method, m_options.target), asio::use_awaitable);
	}

	asio::awaitable<void> async_http_client::perform_request(request_method method, const std::string_view target)
	{
		using namespace boost::beast;

		request_t request;
		request.version(11); // http version
		request.method(method);
		request.target(target.data());

		request.set(http::field::host, m_host.data());
		request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		co_await http::async_write(m_socket, request, asio::use_awaitable);

		co_await asio::co_spawn(m_context.get_executor(), produce_response(), asio::use_awaitable);
	}

	asio::awaitable<void> async_http_client::produce_response()
	{
		using namespace boost::beast;

		flat_buffer buffer;

		co_await http::async_read(m_socket, buffer, m_response, asio::use_awaitable);

		is_ready_response.store(true);
		m_cv.notify_all();
	}

	std::optional<async_http_client::response_t> async_http_client::consume_response(std::chrono::milliseconds awaiting_time)
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		if (m_cv.wait_for(lk, awaiting_time, [this] {return is_ready_response.load() || has_error.load(); }))
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