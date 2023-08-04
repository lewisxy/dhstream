#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <csignal>

#include "argparse.hpp"
#include "dhnetsdk.h"

#include "dhstream.h"

// signal handler
sig_atomic_t stopFlag = 0;
void handler(int) {
    stopFlag = 1;
}

bool debug = false;

static string ConvertLoginError2String(int errorCode) {
	switch (errorCode) {
	case 0: 
		return "Login Success";

	case 1: 
		return "Account or Password Incorrect";

	case 2: 
		return "User Is Not Exist";

	case 3: 
		return "Login Timeout";

	case 4: 
		return "Repeat Login";

	case 5:
		return "User Account is Locked";

	case 6:
		return "User In Blocklist";

	case 7:
		return "Device Busy";

	case 8: 
		return "Sub Connect Failed";

	case 9:
		return "Host Connect Failed";
	
	case 10:
		return "Max Connect";

	case 11:
		return "Support Protocol3 Only";

	case 12:
		return "UKey Info Error";

	case 13:
		return "No Authorized";
	
	case 18:
		return "Device Account isn't Initialized";

	default:
        return "Unknown Error";
	}
}

static std::optional<DH_RealPlayType> getRealplayType(int streamtype) {
    switch (streamtype) {
	case 0: 
		return DH_RType_Realplay;

	case 1: 
		return DH_RType_Realplay_1;

	case 2: 
		return DH_RType_Realplay_2;

	case 3: 
		return DH_RType_Realplay_3;

	default:
        // invalid type
        return std::nullopt;
	}
}

// assume dwUser is pointer to ostream object
void CALLBACK RealDataCallBackEx(LLONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, LLONG param, LDWORD dwUser) {
    std::ostream* out = (std::ostream*) dwUser;
    if (debug) {
        std::cerr << "received buffer, handle " << lRealHandle << ", size " << dwBufSize << std::endl;
    }
    out->write((char*) pBuffer, dwBufSize);
}

void CALLBACK DisConnectFunc(LLONG lLoginID, char *pchDVRIP, LONG nDVRPort, LDWORD dwUser) {
    std::cerr << "disconnected" << std::endl;
}

void CALLBACK  HaveReConnectFunc(LLONG lLoginID, char *pchDVRIP, LONG nDVRPort, LDWORD dwUser) {
    std::cerr << "reconnected" << std::endl;
}

bool Client::Login(const string& address, const int port,
                    const string& username, const string& password) {
    NET_IN_LOGIN_WITH_HIGHLEVEL_SECURITY stInparam;
    memset(&stInparam, 0, sizeof(stInparam));
    stInparam.dwSize = sizeof(stInparam);
    strncpy(stInparam.szIP, address.data(), sizeof(stInparam.szIP) - 1);
    strncpy(stInparam.szPassword, password.data(), sizeof(stInparam.szPassword) - 1);
    strncpy(stInparam.szUserName, username.data(), sizeof(stInparam.szUserName) - 1);
    stInparam.nPort = port;
    stInparam.emSpecCap = EM_LOGIN_SPEC_CAP_TCP;

    NET_OUT_LOGIN_WITH_HIGHLEVEL_SECURITY stOutparam;
    memset(&stOutparam, 0, sizeof(stOutparam));
    stOutparam.dwSize = sizeof(stOutparam);
    loginHandle_ = CLIENT_LoginWithHighLevelSecurity(&stInparam, &stOutparam);
    
    string errorStr = ConvertLoginError2String(stOutparam.nError);
    if (loginHandle_ == 0) {
        // login failed
        if (stOutparam.nError != 255) {
            std::cerr << "Login failed! Error code: " << stOutparam.nError << ": " << errorStr;
        } else {
            stOutparam.nError = CLIENT_GetLastError();
            if (stOutparam.nError == NET_ERROR_MAC_VALIDATE_FAILED) {
                std::cerr << "Login failed! bad mac address";
            } else if(stOutparam.nError == NET_ERROR_SENIOR_VALIDATE_FAILED) {
                std::cerr << "Login failed! senior validate failed";
            } else {
                std::cerr << "Login failed! unknown error";
            }
        }
        std::cerr << std::endl;
        return false;
    }
    return true;
}

bool Client::Realplay(const int channel, const int streamtype) {
    if (auto type = getRealplayType(streamtype)) {
        realplayHandle_ = CLIENT_RealPlayEx(loginHandle_, channel, NULL, *type);
        if (realplayHandle_ == 0) {
            std::cerr << "realplay failed" << std::endl;
            return false;
        }
        CLIENT_SetRealDataCallBackEx2(realplayHandle_, RealDataCallBackEx, (LDWORD) &out_, REALDATA_FLAG_RAW_DATA);
        return true;
    } else {
        std::cerr << "invalid stream type" << std::endl;
        return false;
    }
}

Client::Client(ostream& out)
        : out_(out), loginHandle_(0), realplayHandle_(0) {
    CLIENT_Init(DisConnectFunc, (LDWORD) this);
    CLIENT_SetAutoReconnect(HaveReConnectFunc, 0);
}

Client::~Client() {
    if (debug) {
        std::cerr << "client destruct" << std::endl;
    }
    if (loginHandle_) {
        if (realplayHandle_) {
            CLIENT_StopRealPlayEx(realplayHandle_);
        }
        CLIENT_Logout(loginHandle_);
    }
    CLIENT_Cleanup();
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("dhstream");

    program.add_argument("address")
        .help("the IP address of server");

    program.add_argument("port")
        .help("the port to connect to")
        .scan<'i', int>();

    program.add_argument("-u", "--username")
        .default_value(std::string("admin"))
        .help("specify the username");

    program.add_argument("-p", "--password")
        .default_value(std::string("password"))
        .help("specify the password");

    program.add_argument("-c", "--channel")
        .default_value(0)
        .help("the channel to use")
        .scan<'i', int>();

    program.add_argument("-s", "--stream-type")
        .default_value(0)
        .help("the stream type to use")
        .scan<'i', int>();

    program.add_argument("--debug")
        .default_value(false)
        .implicit_value(true)
        .help("enable debug message");

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    auto address = program.get<std::string>("address");
    auto port = program.get<int>("port");
    auto username = program.get<std::string>("--username");
    auto password = program.get<std::string>("--password");
    auto channel = program.get<int>("--channel");
    auto streamtype = program.get<int>("--stream-type");
    debug = program.get<bool>("--debug");

    std::cout.setf(std::ios::unitbuf);
    Client c(std::cout);
    if (c.Login(address, port, username, password) && c.Realplay(channel, streamtype)) {
        // sleep indefinitely (while the thread is doing work)
        signal(SIGINT, &handler);
        signal(SIGTERM, &handler);
        signal(SIGQUIT, &handler);
        while (!stopFlag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else {
        return 1;
    }

    return 0;
}
