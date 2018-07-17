deps_config := \
	/home/manue/esp/esp-idf/components/app_trace/Kconfig \
	/home/manue/esp/esp-idf/components/aws_iot/Kconfig \
	/home/manue/esp/esp-idf/components/bt/Kconfig \
	/home/manue/esp/esp-idf/components/driver/Kconfig \
	/home/manue/esp/esp-idf/components/esp32/Kconfig \
	/home/manue/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/manue/esp/esp-idf/components/esp_http_client/Kconfig \
	/home/manue/esp/esp-idf/components/ethernet/Kconfig \
	/home/manue/esp/esp-idf/components/fatfs/Kconfig \
	/home/manue/esp/esp-idf/components/freertos/Kconfig \
	/home/manue/esp/esp-idf/components/heap/Kconfig \
	/home/manue/esp/esp-idf/components/libsodium/Kconfig \
	/home/manue/esp/esp-idf/components/log/Kconfig \
	/home/manue/esp/esp-idf/components/lwip/Kconfig \
	/home/manue/esp/esp-idf/components/mbedtls/Kconfig \
	/home/manue/esp/esp-idf/components/mdns/Kconfig \
	/home/manue/esp/esp-idf/components/openssl/Kconfig \
	/home/manue/esp/esp-idf/components/pthread/Kconfig \
	/home/manue/esp/esp-idf/components/spi_flash/Kconfig \
	/home/manue/esp/esp-idf/components/spiffs/Kconfig \
	/home/manue/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/manue/esp/esp-idf/components/vfs/Kconfig \
	/home/manue/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/manue/esp/esp-idf/Kconfig.compiler \
	/home/manue/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/manue/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/manue/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/manue/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
