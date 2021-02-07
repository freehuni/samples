#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>

#define BUF_LEN 1024

int main()
{
    int fd, wd;
    int len, i;
    char buf[BUF_LEN];

    fd = inotify_init();
    if (fd == -1)
    {
        printf("inotify_init failed\n");
        return 1;
    }

    //wd = inotify_add_watch(fd, "/home/mhkang2/inotify", IN_CREATE | IN_DELETE | IN_MOVE);
    wd = inotify_add_watch(fd, "/media", IN_CREATE | IN_DELETE | IN_MOVE);
    if (fd == -1)
    {
        printf("inotify_add_watch failed\n");
        return 1;
    }

    len = read(fd, buf, BUF_LEN);
    i = 0;
    while(i < len)
    {
        struct inotify_event *event = (struct inotify_event *) &buf[i];
        printf("wd:%d, len:%d, cookie:%d, isDir: %s ",  event->wd, event->len,  event->cookie, (event->mask & IN_ISDIR) ? "yes" : "no");

        //name 필드가 존재한다면 출력
        if(event->len)
        {
            printf(" %s\n", event->name);
        }

        //다음 이벤트 시작 index로 i를 증가시킨다.
        i += sizeof(struct inotify_event) + event->len;
    }

    inotify_rm_watch(fd, wd);
    close(fd);

    return 0;
}

