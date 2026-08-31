<?php
declare(strict_types=1);

namespace Ratatoskr;

final class Dns
{
    private const DECLARATIONS = <<<'C'
        typedef struct ratos_context ratos_context;
        typedef struct ratos_dns_result ratos_dns_result;
        typedef struct ratos_dns_record ratos_dns_record;
        typedef struct ratos_dns_query_options {
          uint32_t struct_size; const char *server; uint16_t port; uint16_t type;
          uint32_t timeout_ms; uint8_t recursion_desired; uint8_t reserved[7];
        } ratos_dns_query_options;
        uint32_t ratos_abi_version(void);
        ratos_context *ratos_context_create(void); void ratos_context_destroy(ratos_context *);
        const char *ratos_context_error(const ratos_context *);
        const char *ratos_error_string(int);
        void ratos_dns_query_options_init(ratos_dns_query_options *);
        int ratos_dns_query(ratos_context *, const char *, const ratos_dns_query_options *, ratos_dns_result **);
        void ratos_dns_result_destroy(ratos_dns_result *);
        uint8_t ratos_dns_result_rcode(const ratos_dns_result *);
        const char *ratos_dns_result_query_name(const ratos_dns_result *);
        uint16_t ratos_dns_result_query_type(const ratos_dns_result *);
        const char *ratos_dns_result_server(const ratos_dns_result *);
        size_t ratos_dns_result_count(const ratos_dns_result *);
        const ratos_dns_record *ratos_dns_result_record(const ratos_dns_result *, size_t);
        uint16_t ratos_dns_record_type_code(const ratos_dns_record *);
        uint8_t ratos_dns_record_section(const ratos_dns_record *);
        const char *ratos_dns_record_name(const ratos_dns_record *);
        uint32_t ratos_dns_record_ttl(const ratos_dns_record *);
        const char *ratos_dns_record_text(const ratos_dns_record *);
        C;

    private static ?\FFI $ffi = null;

    public static function abiVersion(): int { return self::ffi()->ratos_abi_version(); }

    /** @return array{queryName:string,queryType:int,server:string,responseCode:int,records:list<array{typeCode:int,section:int,name:string,ttl:int,text:string}>} */
    public static function query(string $name, DnsRecordType $type = DnsRecordType::A, ?string $server = null, int $port = 53, int $timeoutMs = 5000): array
    {
        if ($name === '' || str_contains($name, "\0")) throw new \InvalidArgumentException('name must not be empty or contain NUL');
        $ffi = self::ffi();
        if ($ffi->ratos_abi_version() !== 1) throw new RatatoskrException('Unsupported Ratatoskr native ABI');
        $context = $ffi->ratos_context_create();
        if (\FFI::isNull($context)) throw new RatatoskrException('Could not allocate native context');
        $result = $ffi->new('ratos_dns_result *');
        try {
            $options = $ffi->new('ratos_dns_query_options');
            $ffi->ratos_dns_query_options_init(\FFI::addr($options));
            $options->server = $server;
            $options->port = $port;
            $options->type = $type->value;
            $options->timeout_ms = $timeoutMs;
            $code = $ffi->ratos_dns_query($context, $name, \FFI::addr($options), \FFI::addr($result));
            if ($code !== 0) {
                $detail = self::text($ffi->ratos_context_error($context));
                throw new RatatoskrException($detail ?: self::text($ffi->ratos_error_string($code)), $code);
            }
            $count = $ffi->ratos_dns_result_count($result);
            if ($count > 4096) throw new RatatoskrException('Native result exceeds safety limit');
            $records = [];
            for ($index = 0; $index < $count; ++$index) {
                $record = $ffi->ratos_dns_result_record($result, $index);
                $records[] = ['typeCode' => $ffi->ratos_dns_record_type_code($record), 'section' => $ffi->ratos_dns_record_section($record), 'name' => self::text($ffi->ratos_dns_record_name($record)), 'ttl' => $ffi->ratos_dns_record_ttl($record), 'text' => self::text($ffi->ratos_dns_record_text($record))];
            }
            return ['queryName' => self::text($ffi->ratos_dns_result_query_name($result)), 'queryType' => $ffi->ratos_dns_result_query_type($result), 'server' => self::text($ffi->ratos_dns_result_server($result)), 'responseCode' => $ffi->ratos_dns_result_rcode($result), 'records' => $records];
        } finally {
            if (!\FFI::isNull($result)) $ffi->ratos_dns_result_destroy($result);
            $ffi->ratos_context_destroy($context);
        }
    }

    private static function ffi(): \FFI
    {
        if (self::$ffi === null) {
            $library = getenv('RATATOSKR_LIBRARY') ?: (PHP_OS_FAMILY === 'Windows' ? 'ratatoskr.dll' : (PHP_OS_FAMILY === 'Darwin' ? 'libratatoskr.dylib' : 'libratatoskr.so'));
            self::$ffi = \FFI::cdef(self::DECLARATIONS, $library);
        }
        return self::$ffi;
    }

    private static function text(\FFI\CData $value): string { return \FFI::isNull($value) ? '' : \FFI::string($value); }
}
