#include "header.h"

using namespace std;


int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9000);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 1);

    cout << "Se asteapta conexiuni pe portul 9000" << endl;

    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

    CryptoPP::Integer p("1234567890123456789012345678901234567890"); 
    CryptoPP::Integer g = 5;

    Party my_party(p, g);

    string my_pub_key_str = integer_to_string(my_party.sendPublicKey());
    send_all(client_socket, my_pub_key_str);

    string client_pub_key_str = receive_all(client_socket);

    // calculeaza shared secret
    CryptoPP::Integer client_pub_key(client_pub_key_str.c_str());
    my_party.create_shared_secret(client_pub_key);

    thread(receive_messages, client_socket, ref(my_party)).detach();

    cout << "Poti incepe sa scrii mesaje:\n> ";
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

    close(server_fd);
    return 0;
}