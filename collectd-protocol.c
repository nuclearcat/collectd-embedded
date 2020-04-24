#include "collectd-protocol.h"
/*
   TODO: encryption, signing, esp-independent code (ESP_LOG, etc)

*/


/* ESP8266 will spit unaligned access otherwise if casting used 
   memcpy can't be used also, due endianness issues
*/
inline static void copy_uint16_t (uint8_t *dst, uint16_t src) {
	dst[0] = (src & 0xFF00) >> 8;
	dst[1] = src & 0x00FF;
}

inline static void copy_uint64_t (uint8_t *dst, uint64_t src) {
	dst[0] = (src & 0xFF00000000000000) >> 56;
	dst[1] = (src & 0x00FF000000000000) >> 48;
	dst[2] = (src & 0x0000FF0000000000) >> 40;
	dst[3] = (src & 0x000000FF00000000) >> 32;
	dst[4] = (src & 0x00000000FF000000) >> 24;
	dst[5] = (src & 0x0000000000FF0000) >> 16;
	dst[6] = (src & 0x000000000000FF00) >> 8;
	dst[7] = src &  0x00000000000000FF;
}

struct collectd_packet *collectd_init_packet(char *hostname, uint16_t len) {
	struct collectd_packet *c = (struct collectd_packet*) malloc(sizeof(struct collectd_packet));
#ifdef EXTRA_SAFETY_CHECK
	if (c == NULL)
		return NULL;
#endif	
	uint32_t required_len = strlen(hostname) + 1 + 4;
	c->current_offset = 0;
	c->available_len = len;
	c->buffer = NULL;
#ifdef EXTRA_SAFETY_CHECK
	if (required_len > c->available_len) {
		free(c);
		return NULL;
	}
#endif
	c->buffer = (uint8_t*)malloc(len);
#ifdef EXTRA_SAFETY_CHECK
	if (c->buffer == NULL) {
		free(c);
		return NULL;
	}
#endif
	copy_uint16_t(&c->buffer[c->current_offset], TYPE_HOST);
	copy_uint16_t(&c->buffer[c->current_offset+2], required_len);
	strcpy((char*)&c->buffer[c->current_offset+4], hostname);
	c->current_offset += required_len;
	c->available_len -= required_len;
	return c;
};

/* Reset existing packet for new packet crafting */
int collectd_reset_packet(struct collectd_packet *c, char *hostname) {
	uint32_t required_len = strlen(hostname) + 1 + 4;
	c->available_len += c->current_offset;
	c->current_offset = 0;
#ifdef EXTRA_SAFETY_CHECK
	if (required_len > c->available_len) {
		return -1;
	}
#endif
	copy_uint16_t(&c->buffer[c->current_offset], TYPE_HOST);
	copy_uint16_t(&c->buffer[c->current_offset+2], required_len);
	strcpy((char*)&c->buffer[c->current_offset+4], hostname);
	c->current_offset += required_len;
	c->available_len -= required_len;
	return 0;
}

int collectd_add_numeric(struct collectd_packet *c, uint16_t type, int64_t value) {
	uint32_t required_len = 12;
#ifdef EXTRA_SAFETY_CHECK
	if (c == NULL || c->buffer == NULL)
		return -1;
#endif
	if (required_len > c->available_len) {
		return -2;
	}
	copy_uint16_t(&c->buffer[c->current_offset], type);
	copy_uint16_t(&c->buffer[c->current_offset+2], required_len);
	copy_uint64_t(&c->buffer[c->current_offset+4], value);
	c->current_offset += required_len;
	c->available_len -= required_len;	
	return 0;
}

int collectd_add_string(struct collectd_packet *c, uint16_t type, char* value) {
	uint32_t required_len = strlen(value) + 1 + 4;
#ifdef EXTRA_SAFETY_CHECK
	if (c == NULL || c->buffer == NULL)
		return -1;
#endif
	if (required_len > c->available_len) {
		return -1;
	}
	copy_uint16_t(&c->buffer[c->current_offset], type);
	copy_uint16_t(&c->buffer[c->current_offset+2], required_len);
	strcpy((char*)&c->buffer[c->current_offset+4], value);
	c->current_offset += required_len;
	c->available_len -= required_len;
	return 0;	
}

/* STUB for now, TODO for handling multiple values */
int collectd_add_values(struct collectd_packet *c, struct collectd_values *v) {
	uint32_t required_len = 6 + v->number * 9;
#ifdef EXTRA_SAFETY_CHECK
	if (c == NULL || c->buffer == NULL)
		return -1;
#endif
	if (required_len > c->available_len) {
		return -1;
	}
	copy_uint16_t(&c->buffer[c->current_offset], TYPE_VALUES);
	copy_uint16_t(&c->buffer[c->current_offset+2], required_len);
	memcpy(&c->buffer[c->current_offset+4], v, v->number * 9);

	c->current_offset += required_len;
	c->available_len -= required_len;
	return 0;	
}

int collectd_add_value(struct collectd_packet *c, uint8_t value_type, void *value_content) {
	uint32_t required_len = 6 + 9;
	uint16_t values_num = 1;
#ifdef EXTRA_SAFETY_CHECK
	if (c == NULL || c->buffer == NULL)
		return -1;	
	if (required_len > c->available_len) {
		return -1;
	}
#endif
	copy_uint16_t(&c->buffer[c->current_offset], TYPE_VALUES);
	copy_uint16_t(&c->buffer[c->current_offset+2], required_len);
	copy_uint16_t(&c->buffer[c->current_offset+4], values_num);
	c->buffer[c->current_offset+6] = value_type;
	memcpy(&c->buffer[c->current_offset+7], value_content, 8);

	c->current_offset += required_len;
	c->available_len -= required_len;
	return 0;	
}

#ifndef ARDUINO
int send_packet (char *dst_ip, int dst_port, struct collectd_packet *c) {
	int addr_family = AF_INET;
	int ip_protocol = IPPROTO_IP;

	struct sockaddr_in destAddr;
	destAddr.sin_addr.s_addr = inet_addr(dst_ip);
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(dst_port);

	int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
	if (sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		return -1;
	}

	int err = sendto(sock, c->buffer, c->current_offset, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
	if (err < 0) {
		ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
		return -2;
	}
	shutdown(sock , 0);
	close(sock);
	return 0;
}
#else
// Sending UDP in arduino is different! It require C++, so it will be just stub
int send_packet (char *dst_ip, int dst_port, struct collectd_packet *c) {
	return 0;
}
#endif

struct collectd_packet *collectd_free_packet(struct collectd_packet *c) {
	if (c->buffer)
		free(c->buffer);
	free(c);
	return NULL;
}