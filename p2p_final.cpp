// p2p_final.cpp — Ultra-compatible Windows P2P file transfer (C++11)
// Build: g++ -std=c++11 p2p_final.cpp -lws2_32 -o p2p.exe

#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

void winsock_init() {
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0) {
        cout << "Winsock init failed\n";
        exit(1);
    }
}

long long file_size(const string &file) {
    HANDLE h = CreateFileA(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    LARGE_INTEGER sz;
    GetFileSizeEx(h, &sz);
    CloseHandle(h);
    return sz.QuadPart;
}

string safe_name(string p) {
    size_t pos = p.find_last_of("/\\");
    if (pos != string::npos) p = p.substr(pos + 1);
    if (p.find("..") != string::npos) p = "unsafe";
    if (p.empty()) p = "file";
    return p;
}

bool send_all(SOCKET s, const char *b, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int n = send(s, b + sent, (int)(len - sent), 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

bool recv_exact(SOCKET s, char *b, size_t len) {
    size_t got = 0;
    while (got < len) {
        int n = recv(s, b + got, (int)(len - got), 0);
        if (n <= 0) return false;
        got += n;
    }
    return true;
}

string recv_line(SOCKET s) {
    string out;
    char c;
    while (true) {
        int n = recv(s, &c, 1, 0);
        if (n <= 0) return "";
        if (c == '\n') break;
        out.push_back(c);
    }
    return out;
}

bool send_line(SOCKET s, string m) {
    if (m.back() != '\n') m += "\n";
    return send_all(s, m.c_str(), m.size());
}

void handle_put(SOCKET c, string name, long long size) {
    string file = safe_name(name);
    ofstream f(file.c_str(), ios::binary);
    if (!f) { send_line(c, "ERR"); return; }

    vector<char> buf(65536);
    long long left = size;
    while (left > 0) {
        size_t chunk = (size_t)min<long long>(left, buf.size());
        if (!recv_exact(c, buf.data(), chunk)) return;
        f.write(buf.data(), chunk);
        left -= chunk;
    }
    send_line(c, "OK");
    cout << "Received " << file << " (" << size << " bytes)\n";
}

void handle_get(SOCKET c, string name) {
    long long size = file_size(name);
    if (size < 0) { send_line(c, "ERR"); return; }

    ifstream f(name.c_str(), ios::binary);
    if (!f) { send_line(c, "ERR"); return; }

    send_line(c, string("SIZE ") + to_string(size));

    vector<char> buf(65536);
    while (f) {
        f.read(buf.data(), buf.size());
        streamsize n = f.gcount();
        if (n > 0) send_all(c, buf.data(), (size_t)n);
    }
    cout << "Sent " << name << " (" << size << " bytes)\n";
}

void server_mode(string port) {
    winsock_init();

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(port.c_str()));

    bind(s, (sockaddr*)&addr, sizeof(addr));
    listen(s, 5);
    cout << "Listening on port " << port << endl;

    while (1) {
        SOCKET c = accept(s, NULL, NULL);
        string line = recv_line(c);
        string cmd, name;
        long long size = 0;
        stringstream ss(line);
        ss >> cmd >> name >> size;
        if (cmd == "PUT") handle_put(c, name, size);
        else if (cmd == "GET") handle_get(c, name);
        closesocket(c);
    }
}

SOCKET connect_to(string ip, string port) {
    winsock_init();
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port.c_str()));
    addr.sin_addr.s_addr = inet_addr(ip.c_str()); // ✅ Never uses DNS

    if (connect(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
        cout << "Connection failed\n";
        exit(1);
    }
    return s;
}

void put_file(string ip, string port, string file, string remote) {
    SOCKET s = connect_to(ip, port);
    long long size = file_size(file);
    if (remote == "") remote = safe_name(file);

    send_line(s, "PUT " + remote + " " + to_string(size));

    ifstream f(file.c_str(), ios::binary);
    vector<char> buf(65536);
    while (f) {
        f.read(buf.data(), buf.size());
        streamsize n = f.gcount();
        if (n > 0) send_all(s, buf.data(), (size_t)n);
    }

    cout << "Server: " << recv_line(s) << endl;
    closesocket(s);
}

void get_file(string ip, string port, string file, string save) {
    SOCKET s = connect_to(ip, port);
    send_line(s, "GET " + file);

    string line = recv_line(s);
    string cmd; long long size;
    stringstream ss(line);
    ss >> cmd >> size;
    if (cmd != "SIZE") { cout << "Error: " << line << endl; return; }

    if (save == "") save = safe_name(file);
    ofstream out(save.c_str(), ios::binary);

    vector<char> buf(65536);
    long long left = size;
    while (left > 0) {
        size_t chunk = (size_t)min<long long>(left, buf.size());
        if (!recv_exact(s, buf.data(), chunk)) break;
        out.write(buf.data(), chunk);
        left -= chunk;
    }

    cout << "Saved " << save << " (" << size << " bytes)\n";
    closesocket(s);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Usage:\n";
        cout << "  p2p listen <port>\n";
        cout << "  p2p put <ip> <port> <file> [remote]\n";
        cout << "  p2p get <ip> <port> <file> [save_as]\n";
        return 0;
    }

    string mode = argv[1];
    if (mode == "listen") server_mode(argv[2]);
    else if (mode == "put" && (argc == 5 || argc == 6))
        put_file(argv[2], argv[3], argv[4], argc == 6 ? argv[5] : "");
    else if (mode == "get" && (argc == 5 || argc == 6))
        get_file(argv[2], argv[3], argv[4], argc == 6 ? argv[5] : "");
}
