#include <string>
#include <memory>

using namespace std;

class LoggerConsole;
class LoggerFile;

class Logger 
{
public:
    Logger()
    {}

    void init();

    void print(string message);

private:
    unique_ptr<LoggerConsole> mImplConsole;
    unique_ptr<LoggerFile> mImplFile;
};

class LoggerConsole
{
public:
    LoggerConsole()
    {}

    void print(string message)
    {
        printf("%s\n", message.c_str());
    }
};

class LoggerFile
{
public:
    LoggerFile(string logName)
    {
        mFile = fopen(logName.c_str(), "w");
        if (mFile == nullptr)
        {
            throw "Not opened file";
        }
    }
    ~LoggerFile()
    {
        if (mFile != nullptr)
        {
            fclose(mFile);
        }
    }

    void print(string message)
    {
        fprintf(mFile, "%s\n", message.c_str());
    }
private:
    FILE* mFile;    
};

void Logger::print(string message)
{
    if (mImplConsole != nullptr) 
    {
        mImplConsole->print(message);
    }

    if (mImplFile != nullptr)
    {
        mImplFile->print(message);
    }
}
void Logger::init()
{
    mImplConsole = make_unique<LoggerConsole>();
    mImplFile = make_unique<LoggerFile>("bridge.log");
}

//==============================================
int main()
{
    Logger log;
    log.init();

    log.print("hello bridge pattern!");
    return 0;
}