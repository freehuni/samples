#include <stdio.h>
#include <iostream>
#include <string>
#include <glib-object.h>
#include <glib.h>

using namespace std;


G_BEGIN_DECLS

enum {
	NAME_CHANGED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

typedef struct
{
	GObject parent;

    gint id;
	char* name;
} Address;

typedef struct
{
	GObjectClass parent_class;
} AddressClass;

G_END_DECLS


G_DEFINE_TYPE(Address, g_address, G_TYPE_OBJECT);

static void g_address_init(Address* addr)
{
    addr->id = -1;
	addr->name = NULL;

	printf("[%s]\n", __FUNCTION__);
}

static GObject*  g_address_constructor(GType gtype, guint n_properties, GObjectConstructParam* properties)
{
	GObject* obj;

	printf("[%s]\n", __FUNCTION__);

	obj = G_OBJECT_CLASS(g_address_parent_class)->constructor(gtype, n_properties, properties);
}

static void g_address_finalize(GObject* self)
{
	Address* addr = G_TYPE_CHECK_INSTANCE_CAST(self, g_address_get_type(), Address);

	printf("[%s]\n", __FUNCTION__);

	if (addr->name != NULL)
	{
		g_free(addr->name);
		addr->name = NULL;
	}

	G_OBJECT_CLASS(g_address_parent_class)->finalize(self);
}

static void g_address_class_init(AddressClass* klass)
{
	GObjectClass* gobject_class;

	printf("[%s]\n", __FUNCTION__);

	gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->constructor	= g_address_constructor;
	gobject_class->finalize		= g_address_finalize;

	signals[NAME_CHANGED] = g_signal_new(
								"name-changed",
								g_address_get_type(),
								G_SIGNAL_RUN_LAST,
								0,
								NULL,
								NULL,
								g_cclosure_marshal_VOID__STRING,
								G_TYPE_NONE,
								1,
								G_TYPE_STRING);
}

Address* g_address_new()
{
	return G_TYPE_CHECK_INSTANCE_CAST(g_object_new(g_address_get_type(),NULL), g_address_get_type(), Address);
}

void g_address_set_name(Address* addr, const char* name)
{
	addr->name = g_strdup(name);
	g_signal_emit(addr, signals[NAME_CHANGED], 0, name);
}

void onNameChanged(Address* addr, gchar* name, gpointer user_data)
{
	printf("[%s] name:%s\n", __FUNCTION__, name);
}

int main()
{
	Address* addr;

	addr =  G_TYPE_CHECK_INSTANCE_CAST(g_object_new(g_address_get_type(), NULL), g_address_get_type(), Address);

	g_signal_connect(addr, "name-changed", G_CALLBACK(onNameChanged), (gpointer)0 /* user_data */);

	g_address_set_name(addr, "Kang Myung Hun");
	g_address_set_name(addr, "Freehuni");

	g_object_unref(addr);

    return 0;
}

