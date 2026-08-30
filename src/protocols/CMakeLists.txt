# Each protocol owns its source list and private implementation details.
add_subdirectory(dns)

set(RATOS_PROTOCOL_SOURCES ${RATOS_DNS_SOURCES} PARENT_SCOPE)
