#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include "TcpAcceptor.h"
#include "mytypes.h"


TcpAcceptor::TcpAcceptor()
{
}

TcpAcceptor::~TcpAcceptor()
{

}

bool TcpAcceptor::Start(int port)
{
    sockaddr_in servaddr;
    LOG_INFO("(+)");

    mTcpPort = port;

    mTcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mTcpSocket == -1)
    {
        LOG_ERROR("socket creation failed...");
        return false;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(9090);

    LOG_INFO("binding port:%d name:%s", port,"airboy");
    LOG_INFO("binding port:%s v:%d", "freehun", 1234);
    debug("binding port:%d name:%s", port,"airboy");
    debug("binding port:%s v:%d", "freehun", 1234);
    // Binding newly created socket to given IP and verification
    if ((bind(mTcpSocket, (sockaddr*)&servaddr, sizeof(servaddr))) != 0)
    {
        LOG_ERROR("socket bind failed...");
        return false;
    }


    // Now server is ready to listen and verification
    if ((listen(mTcpSocket, 5)) != 0)
    {
        LOG_ERROR("Listen failed...");
        return false;
    }

    mThread = thread(_acceptThread, this);

    LOG_INFO("(-)");
    return true;
}

bool TcpAcceptor::Stop()
{
    LOG_INFO("(+)");

    mIsRunning =false;

    if (mThread.joinable())
    {
        mThread.join();
    }

    LOG_INFO("(-)");
    return true;
}

void TcpAcceptor::_acceptThread(TcpAcceptor* self)
{
    LOG_INFO("(+)");

    self->acceptThread();

    LOG_INFO("(-)");
}

void TcpAcceptor::acceptThread()
{
    socklen_t len = sizeof(sockaddr_in);
    sockaddr_in clientaddr;
    int sockClient;

    mIsRunning =true;
    while(mIsRunning)
    {

        // Accept the data packet from client and verification
        sockClient = accept(mTcpSocket, (sockaddr*)&clientaddr, &len);

    }
}
