#pragma once
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>


namespace telegram {
	
    using json = nlohmann::json;

    class Bot {
    public:
        Bot(const std::string& token, const std::string& admin_chat_id);

        json getUpdates(int timeout = 60, int offset = 0);
        bool sendMessage(const std::string& chat_id, const std::string& text);
        bool checkMessage(const std::string& text, std::string& out_status);
        void sendDailyReport(const std::map<std::string, bool>& topic_status);
        void sendHelpMessage(const std::string& chat_id);
        void sendManualReportRequest(const std::string& chat_id);
    private:
        std::string _token;
        std::string _admin_chat_id;
        boost::asio::io_context _ioc;
        boost::asio::ssl::context _ssl_ctx{ boost::asio::ssl::context::tlsv12_client };

        json makeApiRequest(const std::string& method, const json& params = {});
    };
}