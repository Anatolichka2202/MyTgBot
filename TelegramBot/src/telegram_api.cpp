#include <telegram_api.hpp>
#include <boost/beast/ssl.hpp>
#include <iostream>

namespace telegram {

    Bot::Bot(const std::string& token, const std::string& admin_chat_id)
        : _token(token), _admin_chat_id(admin_chat_id) {
    }

    json Bot::makeApiRequest(const std::string& method, const json& params) {
        namespace beast = boost::beast;
        namespace http = beast::http;
        namespace ssl = boost::asio::ssl;
        using tcp = boost::asio::ip::tcp;

        try {
            // Подготовка HTTPS запроса
            ssl::stream<tcp::socket> stream(_ioc, _ssl_ctx);
            tcp::resolver resolver(_ioc);

            // Устанавливаем соединение с api.telegram.org
            auto const results = resolver.resolve("api.telegram.org", "443");
            boost::asio::connect(stream.next_layer(), results);
            stream.handshake(ssl::stream_base::client);

            // Формируем HTTP-запрос
            std::string target = "/bot" + _token + "/" + method;
            http::request<http::string_body> req{ http::verb::post, target, 11 };
            req.set(http::field::host, "api.telegram.org");
            req.set(http::field::content_type, "application/json");
            req.body() = params.dump();
            req.prepare_payload();

            // Отправка запроса
            http::write(stream, req);

            // Чтение ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(stream, buffer, res);

            // Закрытие соединения
            beast::error_code ec;
            stream.shutdown(ec); // Игнорируем ошибку закрытия

            return json::parse(beast::buffers_to_string(res.body().data()));
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка API: " << e.what() << std::endl;
            return nullptr;
        }
    }

    json Bot::getUpdates(int timeout, int offset) {
        json params = {
            {"timeout", timeout},
            {"offset", offset}
        };
        return makeApiRequest("getUpdates", params);
    }

    bool Bot::sendMessage(const std::string& chat_id, const std::string& text) {
        json params = {
            {"chat_id", chat_id},
            {"text", text}
        };
        auto response = makeApiRequest("sendMessage", params);
        return response.contains("ok") && response["ok"].get<bool>();
    }

    bool Bot::checkMessage(const std::string& text, std::string& out_status) {
        if (text.find("в пути") != std::string::npos) {
            out_status = "в пути";
            return true;
        }
        if (text.find("на месте") != std::string::npos) {
            out_status = "на месте";
            return true;
        }
        if (text.find("завтра") != std::string::npos) {
            out_status = "завтра";
            return true;
        }
        return false;
    }

    void Bot::sendDailyReport(const std::map<std::string, bool>& topic_status) {
        std::string report = "Отчет:\n";
        for (const auto& [topic, has_report] : topic_status) {
            if (!has_report) {
                report += "❌ " + topic + " — нет отписи\n";
            }
        }
        sendMessage(_admin_chat_id, report);
    }
    void Bot::sendHelpMessage(const std::string& chat_id) {
        std::string help_text =
            "🤖 *Что я умею:*\n"
            "/start - Показать это меню\n"
            "/report - Запросить отчет вручную\n"
            "/help - Справка\n\n"
            "Я автоматически проверяю чаты на ключевые фразы:\n"
            "• `в пути`\n"
            "• `на месте`\n"
            "• `завтра @username`";

        json params = {
            {"chat_id", chat_id},
            {"text", help_text},
            {"parse_mode", "Markdown"}
        };
        makeApiRequest("sendMessage", params);
    }
    void Bot::sendManualReportRequest(const std::string& chat_id) {
        json params = {
            {"chat_id", chat_id},
            {"text", "Запрос отчета... Проверяю чаты."},
            {"reply_markup", R"({
            "keyboard": [[{"text": "Да, сформировать отчет"}]],
            "resize_keyboard": true,
            "one_time_keyboard": true
        })"_json}
        };
        makeApiRequest("sendMessage", params);
    }
} // namespace telegram
