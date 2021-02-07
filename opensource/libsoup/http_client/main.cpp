#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libsoup/soup.h>

void sendSample1()
{
    GError *error = NULL;
    SoupMessage* msg;
    SoupSession* session;
    GInputStream* stream;
    GCancellable cancellable;
    char buffer[10000]="no-data";
    gssize size;

    session = soup_session_new();

    msg = soup_message_new("GET", "http://192.168.0.1:52504/etc/linuxigd/gatedesc.xml");

    // append your headers and it's values.
    soup_message_headers_append(msg->request_headers, "head_name", "head_value");

    stream = soup_session_send(session, msg, NULL, &error);
    if (stream != NULL)
    {
        do {
        size = g_input_stream_read(stream, buffer, 5000, NULL, &error);
        printf("[\n%s\n]\n\n",buffer);
        } while(size > 0);
    }
    else
    {
        printf("stream is null\n");
    }
}

static void my_callback(GObject* object, GAsyncResult* result, gpointer user_data)
{
    SoupSession* session = SOUP_SESSION(object);
    GInputStream *stream;
    GError *error = NULL;
    GMainLoop* loop = (GMainLoop*)user_data;
    char buffer[10000]="no-data";
    gssize size;

    printf("[%s:%d]\n", __FUNCTION__, __LINE__);

    stream = soup_session_send_finish (session, result, &error);

    if (stream != NULL)
    {
        do {
            size = g_input_stream_read(stream, buffer, 5000, NULL, &error);
            printf("[\n%s\n]\n\n",buffer);
        } while(size > 0);

        g_object_unref(stream);
    }

    g_main_loop_quit(loop);
}

void sendSample2()
{
    GError *error = NULL;
    SoupMessage* msg;
    SoupSession* session;
    GInputStream* stream;
    GCancellable cancellable;

    guint status;
    GMainLoop* loop;

    session = soup_session_new();

    msg = soup_message_new("GET", "http://192.168.0.1:52504/etc/linuxigd/gatedesc.xml");


    loop = g_main_loop_new(NULL, TRUE);

    soup_session_send_async(session, msg, NULL, my_callback, loop);

    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_object_unref(msg);
    g_object_unref(session);
}

void sendSample3()
{
    GError *error = NULL;
    SoupSession* session;
    GInputStream* stream;
    GCancellable cancellable;
    SoupRequest* request;
    char buffer[10000]="no-data";
    gssize size;
    guint status;


    session = soup_session_new();

    request = soup_session_request(session, "http://192.168.0.1:52504/etc/linuxigd/gatedesc.xml", &error);
    if (request == NULL)
    {
        printf("request is null\n");
    }

    stream = soup_request_send(request, NULL, &error);
    if (stream != NULL)
    {
        do {
            size = g_input_stream_read(stream, buffer, 5000, NULL, &error);
            printf("[\n%s\n]\n\n",buffer);
        } while(size > 0);

        g_object_unref(stream);
    }

    g_object_unref(request);
    g_object_unref(session);

}

int main (int argc, char **argv)
{
    g_type_init();

    sendSample1();
    //sendSample2();
    //sendSample3();


    return 0;
}

