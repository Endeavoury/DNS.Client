#pragma once

#include <ratatoskr/ratatoskr.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ratatoskr {
enum class record_type : std::uint16_t { a = 1, ns = 2, cname = 5, soa = 6, ptr = 12, mx = 15, txt = 16, aaaa = 28, srv = 33, naptr = 35, caa = 257 };
struct query_options { std::string server; std::uint16_t port = 53; std::uint32_t timeout_ms = 5000; bool recursion_desired = true; };
struct record { std::uint16_t type_code; std::uint8_t section; std::string name; std::uint32_t ttl; std::string text; };
struct result { std::string query_name; std::uint16_t query_type; std::string server; std::uint8_t response_code; std::vector<record> records; };

class error : public std::runtime_error {
public:
    error(ratos_error code, const std::string &message) : std::runtime_error(message), code_(code) {}
    [[nodiscard]] ratos_error code() const noexcept { return code_; }
private:
    ratos_error code_;
};

inline result query(const std::string &name, record_type type = record_type::a, const query_options &options = {}) {
    if (ratos_abi_version() != RATOS_ABI_VERSION) throw error(RATOS_ERROR_UNSUPPORTED, "unsupported Ratatoskr native ABI");
    ratos_context *context = ratos_context_create();
    if (!context) throw std::bad_alloc();
    ratos_dns_query_options native_options;
    ratos_dns_query_options_init(&native_options);
    native_options.server = options.server.empty() ? nullptr : options.server.c_str();
    native_options.port = options.port;
    native_options.timeout_ms = options.timeout_ms;
    native_options.type = static_cast<ratos_dns_type>(type);
    native_options.recursion_desired = options.recursion_desired ? 1 : 0;
    ratos_dns_result *native_result = nullptr;
    const ratos_error code = ratos_dns_query(context, name.c_str(), &native_options, &native_result);
    if (code != RATOS_OK) {
        const char *detail = ratos_context_error(context);
        std::string message = detail && *detail ? detail : ratos_error_string(code);
        ratos_context_destroy(context);
        throw error(code, message);
    }
    result value{ratos_dns_result_query_name(native_result), ratos_dns_result_query_type(native_result), ratos_dns_result_server(native_result), ratos_dns_result_rcode(native_result), {}};
    const std::size_t count = ratos_dns_result_count(native_result);
    if (count > 4096) { ratos_dns_result_destroy(native_result); ratos_context_destroy(context); throw error(RATOS_ERROR_PROTOCOL, "native result exceeds safety limit"); }
    value.records.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const ratos_dns_record *item = ratos_dns_result_record(native_result, index);
        value.records.push_back({ratos_dns_record_type_code(item), ratos_dns_record_section(item), ratos_dns_record_name(item), ratos_dns_record_ttl(item), ratos_dns_record_text(item)});
    }
    ratos_dns_result_destroy(native_result);
    ratos_context_destroy(context);
    return value;
}
} // namespace ratatoskr
