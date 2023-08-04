#ifndef DHSTREAM_H
#define DHSTREAM_H

#include <iostream>
#include <fstream>
#include <string>

#include "dhnetsdk.h"

using namespace std;

class Client {
public:
    Client(ostream& out);
    ~Client();

    bool Login(const string& address, const int port,
            const string& username, const string& password);
    bool Realplay(const int channel, const int streamtype);
private:
    ostream& out_;

    LLONG loginHandle_;
    LLONG realplayHandle_;
};

#endif // DHSTREAM_H