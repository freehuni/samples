/**
 * @file main.cpp
 * @brief
 */

#include <iostream>
#include <string>
#include <glib-object.h>
#include <string.h>
#include <stdio.h>

using namespace std;


G_BEGIN_DECLS

enum {
		OBJECT_AVAILABLE,
		ITEM_AVAILABLE,
		CONTAINER_AVAILABLE,
		SIGNAL_LAST
};
// property id
enum
{
	G_ADDRESS_PROP_0 = 0,
	G_ADDRESS_PROP_ID,
	G_ADDRESS_PROP_NAME,
	G_ADDRESS_PROP_PHONE
};

// signal id
enum
{
  SIG_TEST,
  LAST_SIGNAL
};

static guint Signals[LAST_SIGNAL] = { 0 };
static guint signals[SIGNAL_LAST];

typedef struct
{
	// fixed!
	GObject parent;

	// add public member
	int  public_id;
	gchar* public_str;
}Address;


typedef struct
{
	// fixed
	GObjectClass parent_class;

	// signale
	void (* object_available)    (Address    *address);
} AddressClass;


typedef struct
{
	gint   id;  /*! id정보를 표시합니다. */
	gchar* name;
	gchar* phone;

} AddressPrivate;

G_END_DECLS



/**
 * @brief 이게 핵심!!! 여기서 모든것이 자동 선언된다.
 */
G_DEFINE_TYPE(Address,g_address, G_TYPE_OBJECT);

/**
G_TYPE_CHECK_INSTANCE_CAST  : gobject type으로부터 c type으로 type casting
G_TYPE_INSTANCE_GET_PRIVATE : c type으로부터 private space addr얻어오기
*/
static GObject* g_address_constructor(GType gtype, guint n_properties, GObjectConstructParam * properties);
static void     g_address_init(Address* addr);
static void     g_address_finalize(GObject* self);
static void     g_address_class_init(AddressClass* klass);

static void     g_address_set_property(GObject * object, guint property_id, const GValue* value, GParamSpec* pspec);
static void     g_address_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* psec);

void            g_address_set_name(Address* addr, gchar* name);
gchar*          g_address_get_name(Address* addr);

static void     property_name_notified(GObject* object, GParamSpec* pspec, gpointer user_data);
static void     property_phone_notified(GObject* object, GParamSpec* pspec, gpointer user_data);
static void     property_sig_test_notified(Address* addr, gint value);


//  Implementation ====================================================

static void g_address_set_property(GObject * object, guint property_id, const GValue* value, GParamSpec* pspec)
{
	Address* addr = G_TYPE_CHECK_INSTANCE_CAST(object, g_address_get_type(), Address);
	AddressPrivate* priv;
	priv = G_TYPE_INSTANCE_GET_PRIVATE(addr, g_address_get_type(), AddressPrivate);

	switch(property_id)
	{
	case G_ADDRESS_PROP_ID:
		priv->id = g_value_get_int(value);
		break;

	case G_ADDRESS_PROP_NAME:
		g_free(priv->name);
		priv->name = g_value_dup_string(value);
		break;

	case G_ADDRESS_PROP_PHONE:
		g_free(priv->phone);
		priv->phone = g_value_dup_string(value);
		break;
	}
}

void g_address_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* psec)
{
	Address* addr = G_TYPE_CHECK_INSTANCE_CAST(object, g_address_get_type(), Address);
	AddressPrivate* priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(addr, g_address_get_type(), AddressPrivate);

	switch(property_id)
	{
	case G_ADDRESS_PROP_ID:
		g_value_set_int(value, priv->id);
		break;

	case G_ADDRESS_PROP_NAME:
		g_value_set_string(value, priv->name);
		break;
	case G_ADDRESS_PROP_PHONE:
		g_value_set_string(value, priv->phone);
		break;
	}
}


// g_object_new 으로 object 생성시 호출되는 함수
static void g_address_init(Address* addr)
{
	AddressPrivate* priv;

	printf("%s\n", __FUNCTION__);

	addr->public_id  = -1;
	addr->public_str = NULL;


	priv = G_TYPE_INSTANCE_GET_PRIVATE(addr, g_address_get_type(), AddressPrivate);

	priv->id    = 0;
	priv->name  = NULL;
	priv->phone = NULL;
}

// ref.count=0가 되면 호출되는 함수. 할당받았던 메모리 해제시킨다.
static void g_address_finalize(GObject* self)
{
	AddressPrivate* priv;
	Address* addr = G_TYPE_CHECK_INSTANCE_CAST(self, g_address_get_type(), Address);

	printf("%s\n", __FUNCTION__);

	priv = G_TYPE_INSTANCE_GET_PRIVATE(addr, g_address_get_type(), AddressPrivate);

	g_free(priv->name);
	g_free(priv->phone);

	G_OBJECT_CLASS(g_address_parent_class)->finalize(self);
}

/**
 * constructor, finalize, property, overriding 설정 및 private 영역 할당.
 * @brief g_address_class_init
 * @param klass
 */
static void g_address_class_init(AddressClass* klass)
{
	GObjectClass* gobject_class;
	GParamSpec* pspec;

	printf("%s\n", __FUNCTION__);

	gobject_class = G_OBJECT_CLASS(klass);

	// set constructor
	gobject_class->constructor = g_address_constructor;

	// set object finalizer
	gobject_class->finalize =g_address_finalize;

	// add space of AddressPrivate struct.
	g_type_class_add_private(gobject_class, sizeof(AddressPrivate));

	// set  property
	gobject_class->set_property = g_address_set_property;
	gobject_class->get_property = g_address_get_property;

	pspec = g_param_spec_int (
				"id",
				"ID",
				"Nick of id",
				0   /*minimum*/,
				999 /*maximum*/,
				0   /*default_value*/,
				(GParamFlags)G_PARAM_READWRITE);

	g_object_class_install_property(gobject_class,
									G_ADDRESS_PROP_ID,
									pspec);

	pspec = g_param_spec_string(
				"name",
				"Name",
				"the name of a people",
				"",
				(GParamFlags)G_PARAM_READWRITE);

	g_object_class_install_property(gobject_class,
									G_ADDRESS_PROP_NAME,
									pspec);

	pspec = g_param_spec_string(
				"phone",
				"Phone",
				"the dial number of phone",
				"",
				(GParamFlags)G_PARAM_READWRITE);

	g_object_class_install_property(gobject_class,
									G_ADDRESS_PROP_PHONE,
									pspec);


	// signal test
	Signals[SIG_TEST] =
		g_signal_new ("sig_test",
			  G_OBJECT_CLASS_TYPE (gobject_class),
			  (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__INT,
			  G_TYPE_NONE, 1, G_TYPE_INT);

	signals[OBJECT_AVAILABLE] =
			g_signal_new ("object-available",
						  g_address_get_type(),
						  G_SIGNAL_RUN_LAST,
						  G_STRUCT_OFFSET (AddressClass,
										   object_available),
						  NULL,
						  NULL,
						  g_cclosure_marshal_VOID__OBJECT,
						  G_TYPE_NONE,
						  0);

}
/*
extern void g_cclosure_marshal_VOID__INT (GClosure     *closure,
										  GValue       *return_value,
										  guint         n_param_values,
										  const GValue *param_values,
										  gpointer      invocation_hint,
										  gpointer      marshal_data);
*/


/**
 * g_address_init 함수는 해당 object 자체 값을 초기화하지만, constructor는 parent constructor가 계속 호출된다.
 * @brief g_address_constructor
 * @param gtype
 * @param n_properties
 * @param properties
 * @return
 */
static GObject* g_address_constructor(GType gtype, guint n_properties, GObjectConstructParam * properties)
{
	GObject* obj;

	printf("%s\n", __FUNCTION__);

	obj = G_OBJECT_CLASS(g_address_parent_class)->constructor(gtype, n_properties, properties);
}

// 간편하게 object를 new하자.
Address* g_address_new()
{
	return G_TYPE_CHECK_INSTANCE_CAST(g_object_new(g_address_get_type(),NULL), g_address_get_type(), Address);
}



// property 이용하면 필요없다.

// properties를 이용하면 필요없다.
gchar* g_address_get_name(Address* addr)
{
	AddressPrivate* priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(addr, g_address_get_type(), AddressPrivate);

	return priv->name;
}
void g_address_set_name(Address* addr, gchar* name);


// This function is called when a id property was changed.
static void property_id_notified(GObject* object, GParamSpec* pspec, gpointer user_data)
{
	GValue value={0};
	g_value_init(&value, pspec->value_type);
	g_object_get_property(object, pspec->name, &value);

	printf("[%s] %s: %d\n", __FUNCTION__, pspec->name, g_value_get_int(&value));

}




// This function is called when a phone property was changed.
static void property_phone_notified(GObject* object, GParamSpec* pspec, gpointer user_data)
{
	GValue value={0};
	gchar* str;
	g_value_init(&value, pspec->value_type);
	g_object_get_property(object, pspec->name, &value);
	str = g_strdup_value_contents(&value);

	printf("[%s] %s: %s\n", __FUNCTION__, pspec->name,str);
	g_free(str);
}



// signal test
static void property_sig_test_notified(Address* addr, gint value)
{

	printf("++++++++++++++++[%s value:%d]++++++++++++++++\n", __FUNCTION__, value);
}



void g_address_set_name(Address* addr, gchar* name)
{
	AddressPrivate*priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(addr, g_address_get_type(), AddressPrivate);

	priv->name = strdup(name);

	// 이 함수가 호출되었을때 property_name_notified signal함수가 호출되도록 한다.
	g_object_notify(G_OBJECT(addr), "name");

	g_signal_emit (addr,  signals[OBJECT_AVAILABLE], 0);
}


// This function is called when a name property was changed.
static void property_name_notified(GObject* object, GParamSpec* pspec, gpointer user_data)
{
	GValue value={0};
	gchar* str;
	g_value_init(&value, pspec->value_type);
	g_object_get_property(object, pspec->name, &value);
	str = g_strdup_value_contents(&value);

	printf("[%s] %s: %s\n", __FUNCTION__, pspec->name,str);
	g_free(str);
}

void onObjectAvailable(Address* address, gpointer user_data)
{
	int value = (int)user_data;
	printf("++++++++++++++++[%s value:%d]++++++++++++++++\n", __FUNCTION__, value);
}

int main()
{
	Address* addr;
	GObject* gobj;

	g_type_init();

	printf("new Address\n");
	addr =  G_TYPE_CHECK_INSTANCE_CAST(g_object_new(g_address_get_type(), NULL), g_address_get_type(), Address);

	// a signal related to properties.


	g_signal_connect(addr,
					 "notify::name",
					 G_CALLBACK(property_name_notified),
					 NULL);

	g_signal_connect(addr, "object-available", G_CALLBACK(onObjectAvailable), (gpointer)1234);

	g_address_set_name(addr, "mhkang");



	// ref.count test

	//ref.count++
	g_object_ref(addr);

	//ref.count--
	g_object_unref(addr);

	// ref.count--
	g_object_unref(addr);   // ref.count=0, delete object!

	return 0;
}
