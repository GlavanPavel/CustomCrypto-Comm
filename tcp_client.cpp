#include "header.h"

using namespace std;


int main() {    
    string ip_server;
    cout << "Introdu adresa ip a serverului (enter pentru default): ";
    getline(cin, ip_server);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9000);
    inet_pton(AF_INET, ip_server.c_str(), &serv_addr.sin_addr);

    cout << "Se conecteaza la " << ip_server << "..." << endl;

    if (connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "[-] Eroare de conectare la server." << endl;
        return -1;
    }

    cout << "Conectat la server" << endl;

    CryptoPP::Integer p("1234567890123456789012345678901234567890");
    CryptoPP::Integer g = 5;
    Party my_party(p, g);

    
    // se asteapta cheia publica de la server
    string server_pub_key_str = receive_all(client_socket);
    CryptoPP::Integer server_pub_key(server_pub_key_str.c_str());

    string my_pub_key_str = integer_to_string(my_party.sendPublicKey());
    send_all(client_socket, my_pub_key_str);
    my_party.create_shared_secret(server_pub_key);

    thread(receive_messages, client_socket, ref(my_party)).detach();

    cout << "\nPoti incepe sa scrii mesaje:\n> ";
    while (true) {
        string msg;
        getline(cin, msg);
        if (!msg.empty()) {
            vector<uint32_t> encrypted_uints = my_party.encrypt_message(msg);
            
            vector<uint8_t> bytes = uint32_t_to_byte(encrypted_uints);
            string to_send(bytes.begin(), bytes.end());
            
            send_all(client_socket, to_send);
            cout << "> ";
        }
    }

    close(client_socket);
    return 0;
}