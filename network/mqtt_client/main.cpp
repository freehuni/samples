#include <iostream>
//#include <mosquitto.h>

using namespace std;

int main()
{
	return 0;
}

#if 0
void my_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	int rc = MOSQ_ERR_SUCCESS;

	//fprintf(stderr, "[%s:%d]\n", __FUNCTION__, __LINE__);

	if (!result)
	{
		printf("connected!\n");
	}
	else
	{
		if (result)
		{
			printf( "%s\n", mosquitto_connack_string(result));
		}
	}
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	printf("[%s:%d]\n", __FUNCTION__, __LINE__);
}

void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
	printf("[%s:%d]\n", __FUNCTION__, __LINE__);
}


int main(int argc, char *argv[])
{
	struct mosquitto *mosq = NULL;
	int rc;
	int mqtt_version = MQTT_PROTOCOL_V31;

	mosquitto_lib_init();

	mosq = mosquitto_new("clientid", true, NULL);
	if (mosq == NULL)
	{
		printf("mosquitto_new failed\n");
		goto CLEAN_UP;
	}

	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
	mosquitto_publish_callback_set(mosq, my_publish_callback);

	mosquitto_opts_set(mosq, MOSQ_OPT_PROTOCOL_VERSION, &mqtt_version);

	rc = mosquitto_connect_bind(mosq, "iot.eclipse.org", 1883, 60, 0);

	do
	{

		mosquitto_loop(mosq, -1, 1);
	} while(1);

CLEAN_UP:
	if (mosq != NULL) mosquitto_destroy(mosq);

	mosquitto_lib_cleanup();
	return 0;
}
#endif
