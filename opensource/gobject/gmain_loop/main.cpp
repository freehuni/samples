#include <iostream>
#include <glib.h>

GMainLoop* gLoop = nullptr;
GMainContext* gContext = nullptr;

using namespace std;

static void add_timeout_to_my_thread (guint interval, GSourceFunc func, gpointer  data)
{
  GSource *src;

  src = g_timeout_source_new (interval);
  g_source_set_callback (src, func, data, NULL);
  g_source_attach (src,   g_main_loop_get_context (gLoop));
  g_source_unref (src);
}

static void add_idle_to_my_thread (GSourceFunc func, gpointer data)
{
  GSource *src;

  src = g_idle_source_new ();
  g_source_set_callback (src, func, data, NULL);
  g_source_attach (src,
				   g_main_loop_get_context (gLoop));
  g_source_unref (src);
}

gboolean myIdle(gpointer data)
{
	static uint cnt=0;
	printf("[%s:%d] cnt:%d\n", __FUNCTION__, __LINE__, cnt++);
	return true;
}

gboolean myTimer(gpointer data)
{
	printf("[%s:%d]\n", __FUNCTION__, __LINE__);
	return true;
}

int main()
{
	gContext = g_main_context_new();
	gLoop = g_main_loop_new(gContext, false);

	add_timeout_to_my_thread(1000, myTimer, nullptr);
	//add_idle_to_my_thread( myIdle, nullptr);

	printf("[%s:%d] loop(+)\n", __FUNCTION__, __LINE__);
	g_main_loop_run(gLoop);
	printf("[%s:%d] loop( start)-)\n", __FUNCTION__, __LINE__);

	return 0;
}
