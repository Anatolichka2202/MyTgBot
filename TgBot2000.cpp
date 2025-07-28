#define _CRT_SECURE_NO_WARNINGS  // Отключаем предупреждения безопасности
#define _WIN32_WINNT 0x0601      // Определяем версию Windows
// Тут будет очень много комментариев...
#include <stdio.h>
#include <tgbot/tgbot.h>
#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <mutex> // погуглил - сказали надо ТУДУ: разобраться в этом
#include <cstdlib>
#include <algorithm>
#include <thread> // потоки - разобраться в этом
#include <Windows.h>
#include <set>
#include <vector>

using namespace std;
using std::string;
using namespace TgBot; // так делать не стоит, но я сделал

struct GroupInfo {
    set <int64_t> admins; // ID алминов чата
    string title;           // название чата
    map<int32_t, string> topics;
};

struct Chat_Status //  хранение статусов
{
    string topic_title; // название темы
    bool reported;  // статус отчёта
    int64_t group_id; // ид группы 
};

class ManageBot {               //класс с логикой
private:
    Bot bot;                                // экземпляр
    map <int64_t, GroupInfo> groups; // зареганные группы
    map <pair<int64_t, int32_t>, Chat_Status> chat_statuses;   // статусы, ключ чатИД и топикИД
    mutex status_mutex;                     // мьютекс для безопасности
    thread reportThread;                    // поток 
    bool running = true;                    // статус работы бота

    // функции бота
    
    

    void logMessage(Message::Ptr message) { // функция логирования

        cout << "[" << message->chat->title << " | " << message->messageThreadId
            << "] " << message->from->username << ": "
            << message->text << endl;
    }
    // TODO переписать, топики не поддерживаются тгботом. 
    // 1. ручная регистраци
    // 2. Создать отдельный бот?
    // 3. хранить токен в конфиге (понять только как сделать) 
    // 4. МЕНЬШЕ СПРАШИВАТЬ У ДИПСИК -БОЛЬШЕ ГУГЛИТЬ ^--^
    void register_group(int64_t group_id)
    {
        try {
            string title = bot.getApi().getChat(group_id)->title;
            groups[group_id] = {}
        }
    }

    void checkForKeywords(Message::Ptr message) { // функция чек

        string text = message->text;
        transform(text.begin(), text.end(), text.begin(), ::tolower);

        if (text.find("в пути") != string::npos ||
            text.find("на месте") != string::npos ) 
        {
            lock_guard<mutex> lock(status_mutex);
            int64_t group_id = message->chat->id;
            int32_t topic_id = message->messageThreadId;
            auto key = make_pair(group_id, topic_id); // 

            if (groups.find(group_id) == groups.end()) {
                register_group(group_id);
            }
            if(groups[group_id].topics.find(topic_id) == groups[group_id].topics.end()) {
                register_topic(group_id, topic_id, message);
            }

            // Обновляем статус
            chat_statuses[key].reported = true;
            cout << "Статус обновлен: " << chat_statuses[key].topic_title << endl;
        }
    }


    string generateReport(int64_t group_id) { // функция для формирования отчета
        string report = "Отчёт для группы: " + groups[group_id].title + "\n\n";
        char timeBuf[64];
        time_t now = time(nullptr);
        tm tm;
        localtime_s(&tm, &now);
        strftime(timeBuf, sizeof(timeBuf), "отчет %H:%M\n", &tm);

        string report = "Отчёт для группы: " + groups[group_id].title + "\n" + timeBuf;
        bool hasWarnings = false;

        {
            lock_guard <mutex> lock(status_mutex);
            for (auto& [key, status] : chat_statuses) {
                if (status.group_id == group_id && !status.reported) {
                    report += "!!! " + status.topic_title + " - нет отчета!\n";
                    hasWarnings = true;
                }
                status.reported = false;
            }
        }
        if (!hasWarnings) report += "Все пункты предоставили отчёты";
        return report;
    }
    // Отправка отчетов по расписанию
    void send_scheduled_reports() {
        lock_guard<mutex> lock(status_mutex);
        for (auto& [group_id, group_info] : groups) {
            string report = generateReport(group_id);
            for (int64_t admin_id : group_info.admins) {
                try {
                    bot.getApi().sendMessage(admin_id, report);
                    cout << "Отчет отправлен администратору: " << admin_id << endl;
                }
                catch (const exception& e) {
                    cerr << "Ошибка отправки отчета: " << e.what() << endl;
                }
            }
        }
    }

    void handleMessage(Message::Ptr message) // обработчик сообщений
    {
        if (message->chat->type != Chat::Type::Supergroup || message->text.empty()) return;
        logMessage(message); // логирование
        checkForKeywords(message); // проверка ключевых фраз
    }
    // регистрация админа
    void register_admin(int64_t group_id, int64_t user_id)
    {
        lock_guard<mutex> lock(status_mutex);

        if (groups.find(group_id) == groups.end())
        {
            groups[group_id] = {
                set<int64_t>(),bot.getApi().getChat(group_id)->title
            };
        }

        groups[group_id].admins.insert(user_id);

        string response = "Вы зарегистрирован как админ для группы:\n";
        response += groups[group_id].title + "\n\n";
        response += "Теперь вы будете получать отчеты в личку";

        bot.getApi().sendMessage(user_id, response);
        cout << "Зарегистрирован админ: " << user_id << "для группы " << group_id << endl;
    }

    void handle_command(Message::Ptr message) {
        string command = message->text.substr(1); // Убираем '/'

        if (command == "register") {
            register_admin(message->chat->id, message->from->id);
        }
        else if (command == "report") {
            // Отправка немедленного отчета
            lock_guard<mutex> lock(status_mutex);
            if (groups.find(message->chat->id) != groups.end()) {
                string report = generateReport(message->chat->id);
                bot.getApi().sendMessage(message->chat->id, report);
            }
        }
    }

    void reportWorker() {       // поток автоотчетов
        while (running) {
            time_t now = time(nullptr);
            tm tm;
            localtime_s(&tm, &now);

            if ((tm.tm_hour == 7 && tm.tm_min == 55) ||
                (tm.tm_hour == 9 && tm.tm_min == 0)) {
                send_scheduled_reports();
                this_thread::sleep_for(chrono::minutes(1)); // защита от дублирования
            }
            this_thread::sleep_for(chrono::seconds(30));
        }
    }
public:

    ; ManageBot(const string& token) // конструктор с инициализацией
        : bot(token)
    {
        
            // Обработка всех сообщений
            bot.getEvents().onAnyMessage([this](Message::Ptr msg) {
                // Команды начинаются с '/'
                if (!msg->text.empty() && msg->text[0] == '/') {
                    handle_command(msg);
                }
                else {
                    handleMessage(msg);
                }
                });

        reportThread = thread([this]() {reportWorker(); });
    }

    ~ManageBot() {          // деструктор
        running = false;
        if (reportThread.joinable())
        {
            reportThread.join();
        }
    }

    // start bota
    void start()
    {
        try {
            cout << "Бот запущен и готов к работе!" << endl;
            bot.getApi().deleteWebhook(); // удаляем вебхуки на всякий
            TgLongPoll longPoll(bot);
            while (running) { // запускаем цикл пока бот работает
                longPoll.start();
            }
        }
        catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }
};

pair <string, int> getEnvironmentVariables() {
    // const char* token = getenv("BOT_TOKEN");
    // const char* adminId = getenv("ADMIN_CHAT_ID");
    string token = "8170607088:AAFaUJPvxFjdK0rSGdObX3uMYSIszgz7j6g";
    int64_t adminId = 556259352;
   /* if (!token || !adminId) {
        cerr << "!!! Ошибка: Не заданы переменные окружения!" << endl;
        cerr << "1. Установите BOT_TOKEN и ADMIN_CHAT_ID" << endl;
        cerr << "2. Перезапустите среду разработки" << endl;
        exit(1);
    }
    */
    return { token, adminId};
    
}


int main() {
   
    // настройка консоли для правильных буков
#ifdef _Win32
    SetConsoleOutputCP(CP_UTF8);
#endif
    
    // получение параметров окружения
    auto [token, adminId] = getEnvironmentVariables();

    // создание и запуск
    try {
        ManageBot Oko_bot(token);
        Oko_bot.start();
    }
    catch (const exception& e)
    {
        cerr << "Critical ERROR: " << e.what() << endl;
        return 1;
    }

    return 0;

}