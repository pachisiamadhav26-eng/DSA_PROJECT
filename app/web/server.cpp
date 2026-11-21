#include "replay.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <bits/stdc++.h>

using namespace std;
using namespace leaderboard;

namespace {

constexpr int port = 25353;
constexpr size_t max_header = 16 * 1024;
constexpr size_t max_trace = 2 * 1024 * 1024;

bool send_all(SOCKET client, const string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        int count = send(client, data.data() + sent,
                         static_cast<int>(min<size_t>(data.size() - sent, 64 * 1024)), 0);
        if (count <= 0) return false;
        sent += static_cast<size_t>(count);
    }
    return true;
}

void respond(SOCKET client, int code, const string& label,
             const string& type, const string& body) {
    ostringstream header;
    header << "HTTP/1.1 " << code << ' ' << label << "\r\n"
           << "Content-Type: " << type << "\r\n"
           << "Content-Length: " << body.size() << "\r\n"
           << "Cache-Control: no-store\r\n"
           << "Connection: close\r\n\r\n";
    send_all(client, header.str());
    send_all(client, body);
}

string read_page() {
    ifstream input("app/web/index.html", ios::binary);
    if (!input) throw runtime_error("Run the server from the repository root.");
    return string(istreambuf_iterator<char>(input), istreambuf_iterator<char>());
}

string run_trace(const string& trace, const string& backend) {
    istringstream input(trace);
    ostringstream output;
    BoardFactory factory;
    if (backend == "avl") {
        factory = [](const vector<Score>&) { return make_avl(); };
    } else {
        factory = [](const vector<Score>& scores) { return make_fenwick(scores); };
    }
    run_replay(input, output, factory);
    return output.str();
}

void handle_client(SOCKET client) {
    string request;
    char buffer[8192];
    size_t header_end = string::npos;
    while ((header_end = request.find("\r\n\r\n")) == string::npos) {
        int count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) return;
        request.append(buffer, count);
        if (request.size() > max_header) {
            respond(client, 413, "Too Large", "text/plain", "Request header too large");
            return;
        }
    }

    istringstream headers(request.substr(0, header_end));
    string method, path, version;
    headers >> method >> path >> version;
    string line;
    getline(headers, line);
    size_t content_length = 0;
    while (getline(headers, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t colon = line.find(':');
        if (colon == string::npos) continue;
        string name = line.substr(0, colon);
        transform(name.begin(), name.end(), name.begin(),
                  [](unsigned char ch) { return static_cast<char>(tolower(ch)); });
        if (name == "content-length") {
            try {
                content_length = stoull(line.substr(colon + 1));
            } catch (const exception&) {
                respond(client, 400, "Bad Request", "text/plain", "Invalid Content-Length");
                return;
            }
        }
    }

    if (method == "GET" && path == "/") {
        respond(client, 200, "OK", "text/html; charset=utf-8", read_page());
        return;
    }
    if (method == "GET" && path == "/api/health") {
        respond(client, 200, "OK", "text/plain", "C++ server ready\n");
        return;
    }
    if (method != "POST" ||
        (path != "/api/replay/avl" && path != "/api/replay/fenwick")) {
        respond(client, 404, "Not Found", "text/plain", "Unknown route\n");
        return;
    }
    if (content_length > max_trace) {
        respond(client, 413, "Too Large", "text/plain", "Trace exceeds 2 MB\n");
        return;
    }
    string body = request.substr(header_end + 4);
    while (body.size() < content_length) {
        int count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) return;
        body.append(buffer, count);
    }
    body.resize(content_length);
    try {
        string backend = path.substr(path.find_last_of('/') + 1);
        respond(client, 200, "OK", "text/plain; charset=utf-8",
                run_trace(body, backend));
    } catch (const exception& error) {
        respond(client, 500, "Server Error", "text/plain", error.what());
    }
}

}

int main(int argc, char** argv) {
    int listen_port = port;
    if (argc > 2) {
        cerr << "usage: leaderboard_web [port]\n";
        return 2;
    }
    if (argc == 2) {
        try {
            size_t used = 0;
            int requested = stoi(argv[1], &used);
            if (used != string(argv[1]).size() || requested < 1 || requested > 65535)
                throw invalid_argument("invalid port");
            listen_port = requested;
        } catch (const exception&) {
            cerr << "port must be a number from 1 to 65535\n";
            return 2;
        }
    }
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        cerr << "Winsock initialization failed\n";
        return 1;
    }
    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) {
        cerr << "Could not create server socket\n";
        WSACleanup();
        return 1;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<u_short>(listen_port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
        listen(server, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Could not listen on 127.0.0.1:" << listen_port
             << " (Windows socket error " << WSAGetLastError() << ")\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }
    cout << "Leaderboard C++ server: http://127.0.0.1:" << listen_port << "\n";
    cout.flush();
    while (true) {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET) break;
        try {
            handle_client(client);
        } catch (const exception& error) {
            respond(client, 500, "Server Error", "text/plain", error.what());
        }
        closesocket(client);
    }
    closesocket(server);
    WSACleanup();
    return 0;
}
