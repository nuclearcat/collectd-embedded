xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                    false, true, portMAX_DELAY);
sntp_setoperatingmode(SNTP_OPMODE_POLL);
sntp_setservername(0, "pool.ntp.org");
sntp_init();
while (time(NULL) < 10000) {
  printf(".");
  vTaskDelay(1000 / portTICK_RATE_MS);
}

double temp = sometemperaturesensor();

struct collectd_packet *packet = collectd_init_packet("esp8266", 1000);
if (packet) {
  time_t timestamp = time(NULL);
  printf("Timestamp %ld\r\n", timestamp);

  collectd_add_numeric(packet, TYPE_TIME, timestamp);
  collectd_add_string(packet, TYPE_PLUGIN, "ds18b20");
  collectd_add_string(packet, TYPE_PLUGIN_INSTANCE, "instance0");
  collectd_add_string(packet, TYPE_TYPE, "gauge");
  collectd_add_string(packet, TYPE_TYPE_INSTANCE, "sensor-1");
  collectd_add_value(packet, COLLECTD_VALUETYPE_GAUGE, &temp);
  send_packet("10.255.255.1", 25826, packet);
  packet = collectd_free_packet(packet);
  printf("Packet sent!\r\n");
}
