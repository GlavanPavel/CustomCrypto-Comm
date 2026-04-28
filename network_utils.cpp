#include "header.h"

string integer_to_string(const CryptoPP::Integer& num) {
    stringstream ss;
    ss << num;
    return ss.str();
}

void receive_messages(int sock, Party& party) {
    while (true) {
        string encrypted_raw = receive_all(sock);
        
        if (encrypted_raw.empty()) {
            cout << "\nPartenerul s-a deconectat" << endl;
            close(sock);
            exit(0); 
        }
        
        vector<uint8_t> bytes(encrypted_raw.begin(), encrypted_raw.end());
        vector<uint32_t> encrypted_uints = byte_to_uint32_t(bytes);
        
        string decrypted_msg = party.decrypt_message(encrypted_uints);
        
        cout << "\n[Partener]: " << decrypted_msg << "\n> " << flush;
    }
}


bool send_all(int sock, const string& data) {
    uint32_t len = htonl(data.size()); 
    if (send(sock, &len, sizeof(len), 0) < 0) return false;
    
    size_t total_sent = 0;
    while (total_sent < data.size()) {
        ssize_t sent = send(sock, data.data() + total_sent, data.size() - total_sent, 0);
        if (sent <= 0) return false;
        total_sent += sent;
    }
    return true;
}

string receive_all(int sock) {
    uint32_t network_len;
    int r = recv(sock, &network_len, sizeof(network_len), MSG_WAITALL);
    if (r <= 0) return "";
    
    uint32_t len = ntohl(network_len);
    string data(len, 0);
    
    int received = recv(sock, &data[0], len, MSG_WAITALL);
    
    return (received == (int)len) ? data : "";
}