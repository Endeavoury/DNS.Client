# frozen_string_literal: true

require "fiddle/import"

module Ratatoskr
  module Native
    extend Fiddle::Importer
    library = ENV["RATATOSKR_LIBRARY"] || (Gem.win_platform? ? "ratatoskr.dll" : (/darwin/ =~ RUBY_PLATFORM ? "libratatoskr.dylib" : "libratatoskr.so"))
    dlload library
    Options = struct ["uint32_t struct_size", "void *server", "uint16_t port", "uint16_t type", "uint32_t timeout_ms", "uint8_t recursion_desired", "uint8_t reserved[7]"]
    extern "uint32_t ratos_abi_version()"
    extern "void *ratos_context_create()"
    extern "void ratos_context_destroy(void *)"
    extern "char *ratos_context_error(void *)"
    extern "char *ratos_error_string(int)"
    extern "void ratos_dns_query_options_init(void *)"
    extern "int ratos_dns_query(void *, char *, void *, void *)"
    extern "void ratos_dns_result_destroy(void *)"
    extern "uint8_t ratos_dns_result_rcode(void *)"
    extern "char *ratos_dns_result_query_name(void *)"
    extern "uint16_t ratos_dns_result_query_type(void *)"
    extern "char *ratos_dns_result_server(void *)"
    extern "size_t ratos_dns_result_count(void *)"
    extern "void *ratos_dns_result_record(void *, size_t)"
    extern "uint16_t ratos_dns_record_type_code(void *)"
    extern "uint8_t ratos_dns_record_section(void *)"
    extern "char *ratos_dns_record_name(void *)"
    extern "uint32_t ratos_dns_record_ttl(void *)"
    extern "char *ratos_dns_record_text(void *)"
  end

  TYPES = { a: 1, ns: 2, cname: 5, soa: 6, ptr: 12, mx: 15, txt: 16, aaaa: 28, srv: 33, naptr: 35, caa: 257 }.freeze
  Record = Data.define(:type_code, :section, :name, :ttl, :text)
  Result = Data.define(:query_name, :query_type, :server, :response_code, :records)

  class Error < StandardError
    attr_reader :code
    def initialize(code, message) = (@code = code; super(message))
  end

  def self.abi_version = Native.ratos_abi_version

  def self.query(name, type: :a, server: nil, port: 53, timeout_ms: 5_000)
    raise ArgumentError, "name must not be empty" if name.empty? || name.include?("\0")
    raise Error.new(9, "unsupported native ABI") unless abi_version == 1
    type_code = type.is_a?(Integer) ? type : TYPES.fetch(type.to_s.downcase.to_sym)
    context = Native.ratos_context_create
    raise NoMemoryError, "could not allocate native context" if context.null?
    options = Native::Options.malloc
    Native.ratos_dns_query_options_init(options)
    server_buffer = server && Fiddle::Pointer[server + "\0"]
    options.server = server_buffer || 0
    options.port = port
    options.type = type_code
    options.timeout_ms = timeout_ms
    options.recursion_desired = 1
    output = Fiddle::Pointer.malloc(Fiddle::SIZEOF_VOIDP)
    output[0, Fiddle::SIZEOF_VOIDP] = [0].pack(Fiddle::SIZEOF_VOIDP == 8 ? "Q" : "L")
    code = Native.ratos_dns_query(context, name, options, output)
    if code != 0
      detail = string(Native.ratos_context_error(context))
      raise Error.new(code, detail.empty? ? string(Native.ratos_error_string(code)) : detail)
    end
    address = output[0, Fiddle::SIZEOF_VOIDP].unpack1(Fiddle::SIZEOF_VOIDP == 8 ? "Q" : "L")
    result = Fiddle::Pointer.new(address)
    count = Native.ratos_dns_result_count(result)
    raise Error.new(6, "native result exceeds safety limit") if count > 4096
    records = count.times.map do |index|
      record = Native.ratos_dns_result_record(result, index)
      Record.new(Native.ratos_dns_record_type_code(record), Native.ratos_dns_record_section(record), string(Native.ratos_dns_record_name(record)), Native.ratos_dns_record_ttl(record), string(Native.ratos_dns_record_text(record)))
    end
    Result.new(string(Native.ratos_dns_result_query_name(result)), Native.ratos_dns_result_query_type(result), string(Native.ratos_dns_result_server(result)), Native.ratos_dns_result_rcode(result), records.freeze)
  ensure
    Native.ratos_dns_result_destroy(result) if defined?(result) && result
    Native.ratos_context_destroy(context) if defined?(context) && context && !context.null?
  end

  def self.string(pointer) = pointer.null? ? "" : pointer.to_s
  private_class_method :string
end
