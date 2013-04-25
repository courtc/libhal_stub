#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#ifdef DEBUG
#define LOG_LOCATION "/tmp/libhal_stub.log"
#define DBG(...) fprintf(__VA_ARGS__)
#else
#define DBG(...)
#endif

typedef void DBusConnection;
typedef void DBusError;

static char *libhal_read_hardware_serial(void)
{
	static char serial[64] = "";
	char path[1024];
	char *home;
	FILE *fp;

	if (serial[0])
		return serial;

	home = getenv("HOME");
	if (home == NULL)
		home = "/tmp";
	snprintf(path, sizeof(path), "%s/.hal", home);
	mkdir(path, 0777);
	snprintf(path, sizeof(path), "%s/.hal/serial", home);
	fp = fopen(path, "rt");
	if (fp != NULL) {
		fgets(serial, sizeof(serial), fp);
		fclose(fp);
		strtok(serial, "\r\n ");
		return serial;
	}

	fp = fopen("/proc/sys/kernel/random/boot_id", "rt");
	if (fp == NULL) {
		srand(time(NULL));
		snprintf(serial, sizeof(serial),
				"%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
				rand() & 0xffff, rand() & 0xffff,
				rand() & 0xffff, rand() & 0xffff,
				rand() & 0xffff, rand() & 0xffff,
				rand() & 0xffff, rand() & 0xffff);
	} else {
		fgets(serial, sizeof(serial), fp);
		fclose(fp);
	}

	strtok(serial, "\r\n ");

	fp = fopen(path, "w");
	if (fp != NULL) {
		fprintf(fp, "%s\n", serial);
		fclose(fp);
	}

	return serial;
}

typedef enum {
	LIBHAL_PROPERTY_TYPE_INVALID = 0,
	LIBHAL_PROPERTY_TYPE_INT32   = ((int)'i'),
	LIBHAL_PROPERTY_TYPE_UINT64  = ((int)'t'),
	LIBHAL_PROPERTY_TYPE_DOUBLE  = ((int)'d'),
	LIBHAL_PROPERTY_TYPE_BOOLEAN = ((int)'b'),
	LIBHAL_PROPERTY_TYPE_STRING  = ((int)'s'),
	LIBHAL_PROPERTY_TYPE_REQUEST = ((int)'r'),
	LIBHAL_PROPERTY_TYPE_STRLIST = ((int)(((int)'s')<<8)+('l'))
} LibHalPropertyType;

struct LibHalProperty;
typedef struct LibHalProperty {
	LibHalPropertyType type;
	union {
		void (* request)(struct LibHalProperty *);
		char *string;
		int32_t int32;
		uint64_t uint64;
		double double_;
		bool boolean;
	};
} LibHalProperty;

typedef struct {
	FILE *debug;
	LibHalProperty prop;
} LibHalContext;

LibHalContext *libhal_ctx_new(void)
{
	LibHalContext *ctx;

	ctx = calloc(1, sizeof(LibHalContext));
	if (ctx == NULL)
		return NULL;

#ifdef DEBUG
	ctx->debug = fopen(LOG_LOCATION, "a");
	if (ctx->debug == NULL)
		return NULL;
#endif

	DBG(ctx->debug, "%s\n", __func__);

	return ctx;
}

bool libhal_ctx_init(LibHalContext *ctx, DBusError *error)
{
	DBG(ctx->debug, "%s\n", __func__);
	return true;
}

bool libhal_ctx_free(LibHalContext *ctx)
{
	DBG(ctx->debug, "%s\n", __func__);
#ifdef DEBUG
	fclose(ctx->debug);
#endif
	free(ctx);
	return true;
}

bool libhal_ctx_shutdown(LibHalContext *ctx, DBusError *error)
{
	DBG(ctx->debug, "%s\n", __func__);
	return true;
}

bool libhal_ctx_set_dbus_connection(LibHalContext *ctx, DBusConnection *conn)
{
	DBG(ctx->debug, "%s\n", __func__);
	return true;
}

static void libhal_serial_req(LibHalProperty *prop)
{
	prop->type = LIBHAL_PROPERTY_TYPE_STRING;
	prop->string = libhal_read_hardware_serial();
}

static struct LibHalProperty libhal_serial_prop = {
	.type = LIBHAL_PROPERTY_TYPE_REQUEST,
	.request = libhal_serial_req,
};

static const struct {
	const char *udi;
	const char *key;
	LibHalProperty *property;
} libhal_device_properties[] = {
	{
		"/org/freedesktop/Hal/devices/computer",
		"system.hardware.serial",
		&libhal_serial_prop,
	},
};

#define array_size(x) (sizeof(x)/sizeof((x)[0]))

static LibHalProperty *libhal_device_get_property(LibHalContext *ctx,
		const char *udi, const char *key)
{
	LibHalProperty *ret;
	int i;

	for (i = 0; i < array_size(libhal_device_properties); ++i) {
		if (strcmp(udi, libhal_device_properties[i].udi))
			continue;
		if (strcmp(key, libhal_device_properties[i].key))
			continue;
		ret = libhal_device_properties[i].property;
		if (ret->type == LIBHAL_PROPERTY_TYPE_REQUEST) {
			ret->request(&ctx->prop);
		} else {
			ctx->prop = *ret;
		}

		return &ctx->prop;
	}
	return NULL;
}

LibHalPropertyType libhal_device_get_property_type(LibHalContext *ctx,
		const char *udi, const char *key, DBusError *error)
{
	LibHalProperty *prop;

	DBG(ctx->debug, "%s(%s, %s)\n", __func__, udi, key);
	prop = libhal_device_get_property(ctx, udi, key);
	if (prop == NULL)
		return LIBHAL_PROPERTY_TYPE_INVALID;

	return prop->type;
}

char *libhal_device_get_property_string(LibHalContext *ctx,
		const char *udi, const char *key, DBusError *error)
{
	LibHalProperty *prop;

	DBG(ctx->debug, "%s(%s, %s)\n", __func__, udi, key);
	prop = libhal_device_get_property(ctx, udi, key);
	if (prop == NULL)
		return NULL;

	DBG(ctx->debug, "    = %s\n", prop->string);
	return prop->string;
}

int32_t libhal_device_get_property_int(LibHalContext *ctx,
		const char *udi, const char *key, DBusError *error)
{
	LibHalProperty *prop;

	DBG(ctx->debug, "%s(%s, %s)\n", __func__, udi, key);
	prop = libhal_device_get_property(ctx, udi, key);
	if (prop == NULL)
		return 0;

	DBG(ctx->debug, "    = %d\n", prop->int32);
	return prop->int32;
}

uint64_t libhal_device_get_property_uint64(LibHalContext *ctx,
		const char *udi, const char *key, DBusError *error)
{
	LibHalProperty *prop;

	DBG(ctx->debug, "%s(%s, %s)\n", __func__, udi, key);
	prop = libhal_device_get_property(ctx, udi, key);
	if (prop == NULL)
		return 0;

	DBG(ctx->debug, "    = %llu\n", prop->uint64);
	return prop->uint64;
}

double libhal_device_get_property_double(LibHalContext *ctx,
		const char *udi, const char *key, DBusError *error)
{
	LibHalProperty *prop;

	DBG(ctx->debug, "%s(%s, %s)\n", __func__, udi, key);
	prop = libhal_device_get_property(ctx, udi, key);
	if (prop == NULL)
		return 0.;

	DBG(ctx->debug, "    = %lf\n", prop->double_);
	return prop->double_;
}

char **libhal_manager_find_device_string_match(LibHalContext *ctx,
		const char *key, const char *value,
		int *num_devices, DBusError *error)
{
	DBG(ctx->debug, "%s(%s, %s)\n", __func__, key, value);
	*num_devices = 0;
	return NULL;
}

void libhal_free_string(char *str)
{
	/* Not allocated */
}
