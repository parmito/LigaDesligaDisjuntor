# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c" "Sd.c" "http_client.c" "Debug.c" "RemoteReceiver.c" "Io.c" "app_wifi.c" "Wifi.c" "UartWifi.c" "State.c" "http_server.c" "App.c")
set(COMPONENT_ADD_INCLUDEDIRS " ")

register_component()
