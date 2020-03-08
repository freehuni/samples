#ifndef TCPACCEPTOR_H
#define TCPACCEPTOR_H

#include <thread>
#include <map>
#include <mutex>

using namespace std;

class TcpAcceptor
{
public:
    TcpAcceptor();
    ~TcpAcceptor();

    bool Start(int port);
    bool Stop();

private:
    static void _acceptThread(TcpAcceptor* self);
    void acceptThread();

    int mTcpSocket;
    int mTcpPort;
    bool mIsRunning;
    thread mThread;
};

#endif // TCPACCEPTOR_H
