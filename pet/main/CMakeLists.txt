idf_component_register(SRCS "main.c" "./WIFI/ConnectWIFI.c" "./dns_server/my_dns_server.c" "./web_server/webserver.c" "./driver/hx711/hx711.c" "./driver/motor/motor.c" "./driver/key/key.c" "./driver/ws2812/ws2812.c"
INCLUDE_DIRS "." "./WIFI" "./dns_server" "./web_server"   "./driver/hx711" "./driver/motor" "./driver/key" "./driver/ws2812"
EMBED_FILES "./web_server/index.html"
)