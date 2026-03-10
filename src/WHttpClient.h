#ifndef W_HTTP_CLIENT
#define W_HTTP_CLIENT

#include <AsyncTCP.h>

class SimpleAsyncHTTP {
public:
    void GET(const char* host, const char* path, std::function<void(int, String)> callback) {
        _callback = callback;
        _client = new AsyncClient();
        _client->onData([this](void* arg, AsyncClient* c, void* data, size_t len) {
            // Hier kommt die Antwort an. In einer echten Implementierung
            // müsste der HTTP-Header geparst werden, um das Ende der Antwort zu erkennen.
            String response = String((char*)data).substring(0, len);
            _callback(200, response); // Rückruf mit den Rohdaten
            c->close();
        }, nullptr);
        
        _client->onDisconnect([](void* arg, AsyncClient* c) {
            delete c;
        }, nullptr);
        
        _client->connect(host, 80);
        String request = "GET " + String(path) + " HTTP/1.1\r\n"
                        "Host: " + String(host) + "\r\n"
                        "Connection: close\r\n\r\n";
        _client->write(request.c_str());
    }

private:
    AsyncClient* _client;
    std::function<void(int, String)> _callback;
};

#endif