#include <iostream>
#include <functional>
#include <stdio.h>
#include <string.h>

using namespace std;

class AutoRelease
{
public:
    AutoRelease(function<void(void)> cleanup) : mCleanUp(cleanup)
    {}
    ~AutoRelease()
    {
        mCleanUp();
    }
private:
    function<void(void)> mCleanUp;
};

int main()
{
    FILE* fp = nullptr;
    char* buffer = nullptr;

    AutoRelease autoClean([&fp, &buffer]()
    {
        printf("[%s:%d] auto cleanup\n", __FUNCTION__, __LINE__);
        if (fp != nullptr)
            fclose(fp);

        if (buffer != nullptr)
            free(buffer);
    });

    printf("[%s:%d] file is opened.\n", __FUNCTION__, __LINE__);
    buffer = strdup("Hello World!\n");
    fp = fopen("readme.txt", "w");
    fprintf(fp, "Content:%s", buffer);

    printf("[%s:%d] Bye bye!\n", __FUNCTION__, __LINE__);

    return 0;
}
