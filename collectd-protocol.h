#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>
#include <sys/param.h>
#include <errno.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

//#include "lwipopts.h"
#include "lwip/err.h"
#include <lwip/sockets.h>
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define TYPE_HOST 				0x0000
#define TYPE_TIME 				0x0001
#define TYPE_TIME_HR 			0x0008
#define TYPE_PLUGIN 			0x0002
#define TYPE_PLUGIN_INSTANCE 	0x0003
#define TYPE_TYPE 				0x0004
#define TYPE_TYPE_INSTANCE 		0x0005
#define TYPE_VALUES 			0x0006
#define TYPE_INTERVAL 			0x0007
#define TYPE_INTERVAL_HR 		0x0009

#define COLLECTD_VALUETYPE_COUNTER	0
#define COLLECTD_VALUETYPE_GAUGE	1
#define COLLECTD_VALUETYPE_DERIVE	2
#define COLLECTD_VALUETYPE_ABSOLUTE	3

#ifndef TAG
#define TAG "collectd"
#endif

struct collectd_packet {
	uint8_t *buffer;
	uint16_t available_len;
	uint16_t current_offset;
};

struct collectd_values {
	int number;
	uint8_t *types;
	uint64_t *values;
};

struct collectd_packet *collectd_init_packet(char *hostname, uint16_t len);
int collectd_add_numeric(struct collectd_packet *c, uint16_t type, int64_t value);
int collectd_add_string(struct collectd_packet *c, uint16_t type, char* value);
int collectd_add_values(struct collectd_packet *c, struct collectd_values *v);
int collectd_add_value(struct collectd_packet *c, uint8_t value_type, void *value_content);
int send_packet (char *dst_ip, int dst_port, struct collectd_packet *c);
struct collectd_packet *collectd_free_packet(struct collectd_packet *c);

#ifdef __cplusplus
}
#endif