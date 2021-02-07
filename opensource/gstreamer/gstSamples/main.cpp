#include <iostream>

using namespace std;

#include <stdio.h>
#include <gst/gst.h>

GMainLoop*loop;

static gboolean  bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop *) data;

	g_print("message type:%s\n",GST_MESSAGE_TYPE_NAME(msg));

	switch (GST_MESSAGE_TYPE (msg))
	{
		case GST_MESSAGE_EOS:
		  g_print ("End of stream\n");
		  g_main_loop_quit (loop);
		  break;

		case GST_MESSAGE_ERROR: {
		  gchar  *debug;
		  GError *error;

		  gst_message_parse_error (msg, &error, &debug);
		  g_free (debug);

		  g_printerr ("Error: %s\n", error->message);
		  g_error_free (error);

		  g_main_loop_quit (loop);
		  break;
		}
		default:
		  break;
	}

	return TRUE;
}

void Chap4InitializingGStreamer()
{
	g_print("====[%s]============\n", __FUNCTION__);
	const gchar *nano_str;
	guint major, minor, micro, nano;

	gst_version (&major, &minor, &micro, &nano);
	if (nano == 1)
		nano_str = "(CVS)";
	else if (nano == 2)
		nano_str = "(Prerelease)";
	else
		nano_str = "";

	printf ("This program is linked against GStreamer %d.%d.%d %s\n",
	major, minor, micro, nano_str);
}

void CreatingGstElement()
{
	g_print("====[%s]============\n", __FUNCTION__);
	GstElement* element;
	gchar* name;

	element = gst_element_factory_make("fakesrc", "source");
	if(element == NULL)
	{
		g_print("Failed to create element of type 'fakesrc'\n");
		return;
	}
	else
	{
		g_print("succeed to create element\n");
	}

	g_object_get(G_OBJECT(element), "name", &name, NULL);
	g_print("the name of the element is '%s'\n", name);

	gst_object_unref(GST_OBJECT (element));

}

void MoreAboutElementFactories()
{
	g_print("====[%s]============\n", __FUNCTION__);
	GstElementFactory* factory;

	factory = gst_element_factory_find("fakesrc");
	if (factory == NULL)
	{
		g_print("not found factory of 'fakesrc'\n");
		return;
	}

	g_print("the '%s' element is a member of the category '%s'.\n"
			 "Description: '%s'\n",
			 gst_plugin_feature_get_name (GST_PLUGIN_FEATURE(factory)),
			 gst_element_factory_get_klass (factory),
			 gst_element_factory_get_description(factory));
}

void LinkingElements()
{
	g_print("====[%s]============\n", __FUNCTION__);
	GstElement* pipeline;
	GstElement* source, *filter, *sink;

	pipeline = gst_pipeline_new("my-pipeline");

	source = gst_element_factory_make("fakesrc", "source");
	filter = gst_element_factory_make("identity", "filter");
	sink = gst_element_factory_make("fakesink", "sink");

	gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);

	if (gst_element_link_many(source, filter, sink,NULL) == false)
	{
		g_warning("Failed to link elements!");
	}


	gst_object_unref(GST_OBJECT(pipeline));
}

void playTestVideo()
{
	g_print("====[%s]============\n", __FUNCTION__);
	GstElement* pipeline;
	GstElement* source, *filter, *sink;
	GstBus* bus;

	pipeline = gst_pipeline_new("my-pipeline");

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, bus_call, loop);

	source = gst_element_factory_make("videotestsrc", "source");
	sink = gst_element_factory_make("ximagesink", "sink");


	gst_bin_add_many(GST_BIN(pipeline), source,  sink, NULL);

	if (gst_element_link_many(source, sink,NULL) == false)
	{
		g_warning("Failed to link elements!");
	}

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

}

static void 	on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
	GstPad *sinkpad;
	GstElement *decoder = (GstElement *) data;

	sinkpad = gst_element_get_static_pad (decoder, "sink");
	if (!gst_pad_is_linked (sinkpad))
	{
		gst_pad_link (pad, sinkpad);
	}

	gst_object_unref (sinkpad);
}


void playVideo()
{
	g_print("====[%s]============\n", __FUNCTION__);
	GstElement* pipeline;
	GstElement* source, *filter, *sink;
	GstBus* bus;

	pipeline = gst_pipeline_new("play mpeg2 ts");

	source = gst_element_factory_make("filesrc", "source");
	if (source == NULL)
	{
		g_print("cannot create source\n");
		return;
	}
	g_object_set(G_OBJECT(source), "location", "/home/mhkang2/video/kika.ts", NULL);

	filter = gst_element_factory_make("decodebin2", "filter");
	if (filter == NULL)
	{
		g_print("cannot create filter");
		return;
	}
	sink = gst_element_factory_make("autovideosink", "sink");
	if (sink == NULL)
	{
		g_print("cannot create sink");
		return;
	}

	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

	gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref (bus);

	gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);

	if (gst_element_link (source, filter) == false)
	{
		g_warning("Failed to link elements between source and filter!\n");
		return;
	}

	g_signal_connect (filter, "pad-added", G_CALLBACK (on_pad_added), sink);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);


}

void playBin2()
{
	GstElement* playbin2;
	GstElement* pipeline;
	GstBus* bus;

	pipeline = gst_pipeline_new("my playbin2");

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	if (bus == NULL)
	{
		g_print("cannot create bus\n");
		return;
	}

	gst_bus_add_watch(bus, bus_call, loop);
	gst_object_unref(bus);

	playbin2 = gst_element_factory_make("playbin2", "source");
	if (playbin2 == NULL)
	{
		g_print("cannot create playbin2\n");
		return;
	}
	gst_bin_add_many(GST_BIN(pipeline), playbin2, NULL);
	g_object_set(G_OBJECT(playbin2),"uri","file:///home/mhkang2/video/O-MP2TS_SK-2.mpg", NULL);
	//g_object_set(G_OBJECT(playbin2),"uri","file:///media/mhkang/linuxhdd/eva/eva.mkv", NULL);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void testMyFilter()
{
	GstElement* pipeline;
	GstElement* filesrc;
	GstElement* myfilter;
	gboolean silence;

	filesrc = gst_element_factory_make("filesrc", "aasrc");
	if (filesrc == NULL)
	{
		g_print("cannot create filesrc\n");
		return;
	}
	g_print("filesrc created\n");

	myfilter = gst_element_factory_make("myfilter_name", "filter test");

	if (myfilter == NULL)
	{
		g_print("cannot create myfilter\n");
		return;
	}

	//g_object_set(myfilter, "silent", true, NULL);
	g_object_set(myfilter, "silent", true, NULL);

	g_object_get(myfilter, "silent", &silence, NULL);

	g_print("myfilter created : silence:%d\n", silence);

}

int main (int argc, char *argv[])
{

	GMainContext* context;


	gst_init (&argc, &argv);

	context = g_main_context_new();
	loop = g_main_loop_new(NULL , false);



	//Chap4InitializingGStreamer();
	//CreatingGstElement();
	//MoreAboutElementFactories();
	//LinkingElements();
	//playTestVideo();
	//playVideo();
	playBin2();
	//testMyFilter();


	g_main_loop_run(loop);

	return 0;
}
