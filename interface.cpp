#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include "header.h"
 
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
 
using namespace std;
using namespace ftxui;


int main(){
    Client client;
    auto screen = ScreenInteractive::Fullscreen();
    AppState state;
    mutex state_mutex;

    auto ip_server = Input(&state.server_ip, "IP server(blank - default)");
    auto port_server = Input(&state.server_port, "Port");
    auto username = Input(&state.username, "Username");
    auto btn_connect = Button("Conecteaza-te", [&] {
        if(client.Connect(state.server_ip,stoi(state.server_port),state.username))
        {
            state.connected = true;
            state.connection_status = "Conectat"; 
        }
        else 
        {
            state.connected = false;
            state.connection_status = "Eroare conectare";
        }
    });
    auto users_menu = Menu(&state.users, &state.selected_user);
    auto message_input = Input(&state.message, "Scrie mesaj");
    auto btn_send_message = Button("Trimite mesaj", [&]{
        if(state.message.empty())
            return;
        lock_guard<mutex> lk(state_mutex);
        if(state.users.empty())
            return;
        if (state.selected_user >= (int)state.users.size())
            state.selected_user = 0;
        string target = state.users[state.selected_user];
        client.SendMessage(target,state.message);
        state.messages.push_back({state.username,state.message,target});
        state.message.clear();
    });

    auto file_input = Input(&state.file, "");
    auto btn_send_file = Button("Trimite fisier",[&]{
        if(state.file.empty())
            return;
        lock_guard<mutex> lk(state_mutex);
        if(state.users.empty())
            return;
        string target = state.users[state.selected_user];
        client.SendFile(target,state.file);
    });

    auto server_container = Container::Horizontal({
            ip_server,port_server,username,btn_connect
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
    
    auto render = Renderer(main_container, [&]{
        lock_guard<mutex> lk(state_mutex);
        Elements msg_elements;
        for(const auto& msg: state.messages){
            msg_elements.push_back(
                text("["+msg.from+"]"+msg.text)
            );
        }
        auto messages_box = vbox(move(msg_elements));
        if (state.selected_user >= (int)state.users.size())
            state.selected_user = 0;
        string selected_user = state.users.empty()? "-":state.users[state.selected_user];
        return vbox({
            window(
                text("Server "),
                vbox({
                    hbox({
                    text(" IP: "), ip_server->Render()   | size(WIDTH, EQUAL, 20) | border,
                    text(" Port: "), port_server->Render() | size(WIDTH, EQUAL, 8)  | border,
                    text(" User: "), username->Render()    | size(WIDTH, EQUAL, 14) | border,
                    }),
                    hbox({
                        btn_connect->Render(),text(" "), text("Status: "), text(state.connection_status)
                    })

                })
            ),
            separator(),
            hbox({
                window(
                    text("Utilizatori activi"),
                    users_menu->Render()
                )| vscroll_indicator |frame | size(HEIGHT,EQUAL, 30),
                window(
                    text("Mesaje primite"),
                    messages_box | vscroll_indicator |frame | size(HEIGHT,EQUAL, 30)
                )|flex
            }),
            separator(),
            window(
                text("Destinatar"),
                text(selected_user)
            ),
            window(
                text("Trimite mesaj"),
                hbox({
                    message_input->Render()|flex,
                    text(" "),
                    btn_send_message->Render()
                })
            ),
            window(
                text("Trimite fisier"),
                hbox({
                    file_input->Render()|flex,
                    text(" "),
                    // btn_choose_file->Render(),
                    // text(" "),
                    btn_send_file->Render()
                })
            )
        }) | border;
    });
    
    thread network_thread([&]{
        while(true){
            auto users = client.GetUsers();
            auto msgs = client.GetNewMessages();
            {
            lock_guard<mutex> lk(state_mutex);
            state.users = users;
            for(auto& m: msgs)
                state.messages.push_back(m);
            }
            screen.PostEvent(Event::Custom);
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    });
    network_thread.detach();
    screen.Loop(render);

    return 0;
}