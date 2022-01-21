#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <optional>
#include <chrono>
#include <string_view>

namespace net
{	
	using namespace boost;
	using namespace std::chrono_literals;
	using request_method = beast::http::verb;

	namespace detail
	{
		template <typename Callable, typename...Args>
		void spawn_thread(Callable&& c, Args&&...args)
		{
			std::jthread thread(std::forward<Callable>(c), std::forward<Args>(args)...);
		}
	} //detail

	struct request_options final
	{
		request_method method{};
		std::string_view target{};
	};

	class async_http_client final
	{
	public:
		using request_t = beast::http::request<beast::http::string_body>;
		using response_t = beast::http::response<beast::http::dynamic_body>;

		async_http_client() = default;

		async_http_client(asio::io_context& context, const std::string_view host) :
			m_context{ context },
			m_socket{ asio::make_strand(m_context) },
			m_host{ host }
		{}

		~async_http_client() { m_context.post([this]() {m_socket.close(); }); };

		async_http_client(const async_http_client&) = delete;
		async_http_client& operator=(const async_http_client&) = delete;

		async_http_client(async_http_client&&)noexcept = default;
		async_http_client& operator=(async_http_client&&)noexcept = default;

		void execute();

		void build_request(request_method, const std::string_view) noexcept;

		[[nodiscard]] std::optional<response_t> consume_response(std::chrono::milliseconds awaiting_time = 200ms);

	private:
		asio::awaitable<void> establish_connection(const std::string_view, const std::string_view);
		asio::awaitable<void> perform_request(request_method, const std::string_view);
		asio::awaitable<void> produce_response();

		/* some request options */
		request_options m_options;

		/* some network fields  */
		asio::io_context& m_context;
		asio::ip::tcp::socket m_socket;
		std::string_view m_host;
		response_t m_response;

		/* some multithreading magic */
		std::condition_variable m_cv;
		mutable std::mutex m_mutex;
		std::atomic<bool> is_ready_response{ false };
		std::atomic<bool> has_error{ false };
	};
} // net