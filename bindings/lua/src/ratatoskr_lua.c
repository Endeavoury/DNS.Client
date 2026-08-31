#include <lua.h>
#include <lauxlib.h>
#include <ratatoskr/ratatoskr.h>
#include <stdio.h>

static void set_string(lua_State *state, const char *key, const char *value) {
    lua_pushstring(state, value != NULL ? value : "");
    lua_setfield(state, -2, key);
}

static int ratatoskr_abi(lua_State *state) {
    lua_pushinteger(state, (lua_Integer)ratos_abi_version());
    return 1;
}

static int ratatoskr_query(lua_State *state) {
    const char *name = luaL_checkstring(state, 1);
    ratos_dns_query_options options;
    ratos_context *context;
    ratos_dns_result *result = NULL;
    ratos_error error;
    size_t index;

    ratos_dns_query_options_init(&options);
    options.type = RATOS_DNS_A;
    if (lua_istable(state, 2)) {
        lua_getfield(state, 2, "type");
        if (!lua_isnil(state, -1)) options.type = (ratos_dns_type)luaL_checkinteger(state, -1);
        lua_pop(state, 1);
        lua_getfield(state, 2, "server");
        if (!lua_isnil(state, -1)) options.server = luaL_checkstring(state, -1);
        lua_pop(state, 1);
        lua_getfield(state, 2, "port");
        if (!lua_isnil(state, -1)) options.port = (uint16_t)luaL_checkinteger(state, -1);
        lua_pop(state, 1);
        lua_getfield(state, 2, "timeout_ms");
        if (!lua_isnil(state, -1)) options.timeout_ms = (uint32_t)luaL_checkinteger(state, -1);
        lua_pop(state, 1);
    }
    context = ratos_context_create();
    if (context == NULL) return luaL_error(state, "could not allocate native context");
    error = ratos_dns_query(context, name, &options, &result);
    if (error != RATOS_OK) {
        const char *detail = ratos_context_error(context);
        char message[512];
        snprintf(message, sizeof(message), "%s", detail != NULL && detail[0] != '\0' ? detail : ratos_error_string(error));
        ratos_context_destroy(context);
        return luaL_error(state, "Ratatoskr error %d: %s", (int)error, message);
    }
    lua_createtable(state, 0, 5);
    set_string(state, "query_name", ratos_dns_result_query_name(result));
    lua_pushinteger(state, ratos_dns_result_query_type(result)); lua_setfield(state, -2, "query_type");
    set_string(state, "server", ratos_dns_result_server(result));
    lua_pushinteger(state, ratos_dns_result_rcode(result)); lua_setfield(state, -2, "response_code");
    lua_createtable(state, (int)ratos_dns_result_count(result), 0);
    for (index = 0; index < ratos_dns_result_count(result) && index < 4096; ++index) {
        const ratos_dns_record *record = ratos_dns_result_record(result, index);
        lua_createtable(state, 0, 5);
        lua_pushinteger(state, ratos_dns_record_type_code(record)); lua_setfield(state, -2, "type_code");
        lua_pushinteger(state, ratos_dns_record_section(record)); lua_setfield(state, -2, "section");
        set_string(state, "name", ratos_dns_record_name(record));
        lua_pushinteger(state, ratos_dns_record_ttl(record)); lua_setfield(state, -2, "ttl");
        set_string(state, "text", ratos_dns_record_text(record));
        lua_rawseti(state, -2, (lua_Integer)index + 1);
    }
    lua_setfield(state, -2, "records");
    ratos_dns_result_destroy(result);
    ratos_context_destroy(context);
    return 1;
}

int luaopen_ratatoskr(lua_State *state) {
    static const luaL_Reg functions[] = {{"abi_version", ratatoskr_abi}, {"query", ratatoskr_query}, {NULL, NULL}};
    luaL_newlib(state, functions);
    lua_pushinteger(state, RATOS_DNS_A); lua_setfield(state, -2, "A");
    lua_pushinteger(state, RATOS_DNS_AAAA); lua_setfield(state, -2, "AAAA");
    lua_pushinteger(state, RATOS_DNS_MX); lua_setfield(state, -2, "MX");
    lua_pushinteger(state, RATOS_DNS_PTR); lua_setfield(state, -2, "PTR");
    return 1;
}
