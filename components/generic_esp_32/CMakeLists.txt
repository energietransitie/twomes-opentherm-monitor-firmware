idf_component_register(SRCS "generic_esp_32.c"
                    INCLUDE_DIRS "include" REQUIRES nvs_flash REQUIRES esp-tls REQUIRES esp_http_client
                    REQUIRES wifi_provisioning)
add_compile_definitions(CONFIG_TWOMES_PROV_TRANSPORT_BLE)
add_compile_definitions(CONFIG_TWOMES_CUSTOM_GPIO)