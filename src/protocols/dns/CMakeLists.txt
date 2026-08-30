set(RATOS_DNS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/dns_builder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dns_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dns_packet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dns_parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dns_transport_tcp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dns_transport_udp.c
    PARENT_SCOPE)
