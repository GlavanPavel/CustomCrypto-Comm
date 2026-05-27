#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include "include/Client.h"
 
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
 
using namespace std;
using namespace ftxui;

struct Message{
    string from;
    string text;
    string to;
};

struct AppState{
    string server_ip = "localhost";
    string server_port = "4000";
    string username = "";
    bool connected = false;
    string connection_status = "Neconectat";

    int selected_user = 0;
    vector<string> users;

    vector<Message> messages;
    string message = "";
    string file_path = "";
};
static void add_user_if_missing(vector<string> &users, const string &user, const string &self)
{
    if (user.empty() || user == self)
        return;
    if (find(users.begin(), users.end(), user) == users.end())
    {
        users.push_back(user);
    }
}
int main(){

    auto screen = ScreenInteractive::Fullscreen();
    AppState state;
    mutex state_mutex;
    atomic<bool>app_running(true);
    Looper looper;
    thread looper_thread;

    Client* client = nullptr;
    auto ip_server = Input(&state.server_ip, "IP server(blank - default)");
    auto port_server = Input(&state.server_port, "Port");
    auto username = Input(&state.username, "Username");
    auto btn_connect = Button("Conecteaza-te", [&]{
        lock_guard<mutex> lk(state_mutex);

        if (state.connected) {
            state.connection_status = "Deja conectat";
            return;
        }

        if (state.username.empty()) {
            state.connection_status = "Scrie username";
            return;
        }
        try{
            client = new Client(state.server_ip,state.server_port,&looper);
            client->onMessageReceived = [&](const string& from, const string&text)
            {
                lock_guard<mutex> lk2(state_mutex);
                add_user_if_missing(state.users, from, state.username);
                state.messages.push_back({ from, text, state.username});
                screen.PostEvent(Event::Custom);
            };
            // client->onFileReceived = [&](const string &from, const string &fileName)
            // {
            //     lock_guard<mutex> lk2(state_mutex);
            //     add_user_if_missing(state.users, from, state.username);
            //     state.messages.push_back({from, "Fisier primit", state.username});
            //     screen.PostEvent(Event::Custom);
            // };
            client->onUserListReceived = [&](const vector<string>& users){
                lock_guard<mutex> lk2(state_mutex);
                state.users = users;
                if(state.selected_user >=(int)state.users.size())
                    state.selected_user = 0;
                screen.PostEvent(Event::Custom);
            };

            client->start(state.username);
            if (!looper_thread.joinable()) {
                looper_thread = thread([&] { looper.run();});
            }

            state.connected = true;
            state.connection_status = "Conectat: "+state.username; 
        }
        catch (const std::exception &e)
        {
            state.connected = false;
            state.connection_status = "Eroare conectare";
        } });
    auto btn_quit = Button("Iesire", [&]
    {
        screen.ExitLoopClosure()();
    });

    auto users_menu = Menu(&state.users, &state.selected_user);
    auto message_input = Input(&state.message, "Scrie mesaj");
    auto btn_send_message = Button("Trimite mesaj", [&]
        {
        lock_guard<mutex> lk(state_mutex);
        if(!client){
            state.connection_status = "Conecteaza-te mai intai";
            return;
        }
        if(state.message.empty()){
            state.connection_status = "Alege destinatar";
            return;
        }
        if(state.message.empty())
        {
            return;
        }
        if (state.selected_user >= (int)state.users.size())
            state.selected_user = 0;

        string target = state.users[state.selected_user];
        auto start = chrono::high_resolution_clock::now();
        client->sendMessage(target,state.message);
        auto stop = chrono::high_resolution_clock::now();
        auto us = chrono::duration_cast<chrono::microseconds>(stop-start).count();
        state.connection_status = "Trimis in "+to_string(us)+" ms";
        state.messages.push_back({state.username,state.message,target});
        state.message.clear();
    });

        auto file_input = Input(&state.file_path, "");
    auto btn_send_file = Button("Trimite fisier",[&]{
        lock_guard<mutex> lk(state_mutex);
        if(!client){
            state.connection_status = "Conecteaza-te mai intai";
            return;
        }
        if(state.message.empty()){
            state.connection_status = "Alege destinatar";
            return;
        }
        if(state.message.empty())
        {
            return;
        }
        if (state.selected_user >= (int)state.users.size())
            state.selected_user = 0;

        string target = state.users[state.selected_user];
        //client->sendFile(target,state.file_path);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        state.messages.push_back({state.username, "A trimis un fisier.", target});
        state.file_path.clear();
        
    });

    auto server_container = Container::Horizontal({
            ip_server,port_server,username,btn_connect,btn_quit
        });
    auto message_container = Container::Horizontal({
        message_input, btn_send_message
    });
    auto file_container = Container::Horizontal({
        file_input, btn_send_file
    });
    auto main_container = Container::Vertical({
        server_container, users_menu, message_container, file_container
    });
    
    auto renderer = Renderer(main_container, [&]{
        lock_guard<mutex> lk(state_mutex);
        Elements msg_lines;
        for(const auto& msg: state.messages){
            string line;
            if(msg.from != state.username)
            {
                line = "[ "+msg.from+" ] "+ msg.text;
                msg_lines.push_back(text(line));
            }
        }
        if(msg_lines.empty())
        { msg_lines.push_back(text("Niciun mesaj"));}

        string selected = "-";
        if (!state.users.empty())
        {
            if (state.selected_user < 0 || state.selected_user >= static_cast<int>(state.users.size()))
            {
                state.selected_user = 0;
            }
            selected = state.users[state.selected_user];
        }
        
        return vbox({
            window(
                text("Server "),
                vbox({hbox({
                text(" IP: "),
                ip_server->Render() | size(WIDTH, EQUAL, 20) | border,
                text(" Port: "),
                port_server->Render() | size(WIDTH, EQUAL, 8) | border,
                text(" User: "),
                username->Render() | size(WIDTH, EQUAL, 14) | border,
                }),
                hbox({btn_connect->Render(), text(" "), btn_quit->Render(), text("Status: "), text(state.connection_status)})
            })),
            separator(),
            hbox({
                window( text("Utilizatori activi"), users_menu->Render()) |vscroll_indicator | frame | size(HEIGHT, EQUAL, 18) | size(WIDTH, EQUAL, 28),
                window( text("Mesaje"), vbox(msg_lines) | vscroll_indicator | flex | size(HEIGHT, EQUAL, 18)) |flex}),
            separator(),
            text("Destinatar selectat: " + selected),
                     window(
                        text("Trimite mesaj"),
                        hbox({message_input->Render() | flex,
                        text(" "),
                        btn_send_message->Render()})),
                     window(
                         text("Trimite fisier"),
                         hbox({file_input->Render() | flex,
                               text(" "),
                               btn_send_file->Render()}))}) |
               border;
    });

    screen.Loop(renderer);
    app_running = false;
    looper.stop();
    if (client)
    {
        delete client;
        client = nullptr;
    }

    if (looper_thread.joinable())
        looper_thread.detach();

    return 0;
}