#include <telegram_api.hpp>
#include <chrono>
#include <thread>
#include <map>
#include <tgbot/tgbot.h>
#include <iostream>
using namespace std;

int main() {
    const std::string BOT_TOKEN = "8170607088:AAFaUJPvxFjdK0rSGdObX3uMYSIszgz7j6g";
    const std::string ADMIN_CHAT_ID = "8170607088";

    telegram::Bot bot(BOT_TOKEN, ADMIN_CHAT_ID);
    std::map<std::string, bool> topic_status; // {"Тема1": false, "Тема2": true, ...}
    
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.sendHelpMessage(std::to_string(message->chat->id));
        });
    int last_update_id = 0;

    while (true) {
        try {
            auto updates = bot.getUpdates(60, last_update_id + 1);

            if (!updates.empty() && updates["ok"].get<bool>()) {
                for (const auto& update : updates["result"]) {
                    last_update_id = update["update_id"].get<int>();

                    if (update.contains("message")) {
                        auto msg = update["message"];
                        std::string chat_id = std::to_string(msg["chat"]["id"].get<int>());
                        
                        //std::string text = msg.contains("text") ? msg["text"].get<std::string>() : "";

                        if (msg.contains("text")) {
                            std::string text = msg["text"].get<std::string>();

                            if (text == "/start") { bot.sendHelpMessage(chat_id); } // вызываем бота, он говорит что умеет
                            else if (text == "/report") { bot.sendManualReportRequest(chat_id); } // ручной запрос отчёта
                            else bot.sendHelpMessage(chat_id) // в случае неправильного сообщения справку
                        }
                        std::string status;
                        if (bot.checkMessage(text, status)) {
                            topic_status[msg["chat"]["title"].get<std::string>()] = true;
                        }
                    }
                }
            }

            // Проверка времени для отправки отчета
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&time);

            if ((tm.tm_hour == 7 && tm.tm_min == 55) ||
                (tm.tm_hour == 9 && tm.tm_min == 0) ||
                (tm.tm_hour == 23 && tm.tm_min == 30))
            {
                bot.sendDailyReport(topic_status);
                topic_status.clear(); // Сброс статусов после отчета
                std::this_thread::sleep_for(std::chrono::minutes(1)); // Защита от дублирования
            }

        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    return 0;
}