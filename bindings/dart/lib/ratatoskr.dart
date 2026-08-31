library ratatoskr;

import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

final class _Context extends Opaque {}

final class _Result extends Opaque {}

final class _Record extends Opaque {}

final class _Options extends Struct {
  @Uint32()
  external int structSize;
  external Pointer<Utf8> server;
  @Uint16()
  external int port;
  @Uint16()
  external int type;
  @Uint32()
  external int timeoutMs;
  @Uint8()
  external int recursionDesired;
  @Array(7)
  external Array<Uint8> reserved;
}

enum DnsRecordType {
  a(1),
  ns(2),
  cname(5),
  soa(6),
  ptr(12),
  mx(15),
  txt(16),
  aaaa(28),
  srv(33),
  naptr(35),
  caa(257);

  const DnsRecordType(this.code);
  final int code;
}

final class DnsQueryOptions {
  const DnsQueryOptions(
      {this.server,
      this.port = 53,
      this.timeoutMs = 5000,
      this.recursionDesired = true});
  final String? server;
  final int port;
  final int timeoutMs;
  final bool recursionDesired;
}

final class DnsRecord {
  const DnsRecord(
      {required this.typeCode,
      required this.section,
      required this.name,
      required this.ttl,
      required this.text});
  final int typeCode;
  final int section;
  final String name;
  final int ttl;
  final String text;
}

final class DnsResult {
  const DnsResult(
      {required this.queryName,
      required this.queryType,
      required this.server,
      required this.responseCode,
      required this.records});
  final String queryName;
  final int queryType;
  final String server;
  final int responseCode;
  final List<DnsRecord> records;
}

final class RatatoskrException implements Exception {
  const RatatoskrException(this.code, this.message);
  final int code;
  final String message;
  @override
  String toString() => 'Ratatoskr error $code: $message';
}

final class Ratatoskr {
  Ratatoskr({DynamicLibrary? library})
      : _library = library ?? DynamicLibrary.open(_libraryPath()) {
    _bind();
    if (abiVersion != 1)
      throw const RatatoskrException(9, 'unsupported native ABI');
  }
  final DynamicLibrary _library;
  late final int Function() _abi;
  late final Pointer<_Context> Function() _create;
  late final void Function(Pointer<_Context>) _destroyContext;
  late final Pointer<Utf8> Function(Pointer<_Context>) _contextError;
  late final Pointer<Utf8> Function(int) _errorString;
  late final void Function(Pointer<_Options>) _init;
  late final int Function(Pointer<_Context>, Pointer<Utf8>, Pointer<_Options>,
      Pointer<Pointer<_Result>>) _query;
  late final void Function(Pointer<_Result>) _destroyResult;
  late final int Function(Pointer<_Result>) _rcode, _queryType;
  late final Pointer<Utf8> Function(Pointer<_Result>) _queryName, _server;
  late final int Function(Pointer<_Result>) _count;
  late final Pointer<_Record> Function(Pointer<_Result>, int) _record;
  late final int Function(Pointer<_Record>) _recordType,
      _recordSection,
      _recordTtl;
  late final Pointer<Utf8> Function(Pointer<_Record>) _recordName, _recordText;

  int get abiVersion => _abi();

  DnsResult query(String name, DnsRecordType type,
      {DnsQueryOptions options = const DnsQueryOptions()}) {
    if (name.isEmpty || name.contains('\u0000'))
      throw ArgumentError.value(name, 'name');
    final context = _create();
    if (context == nullptr)
      throw const RatatoskrException(3, 'could not allocate native context');
    final nativeOptions = calloc<_Options>();
    final output = calloc<Pointer<_Result>>();
    final nativeName = name.toNativeUtf8();
    final nativeServer = options.server?.toNativeUtf8();
    try {
      _init(nativeOptions);
      nativeOptions.ref.server = nativeServer ?? nullptr;
      nativeOptions.ref.port = options.port;
      nativeOptions.ref.type = type.code;
      nativeOptions.ref.timeoutMs = options.timeoutMs;
      nativeOptions.ref.recursionDesired = options.recursionDesired ? 1 : 0;
      final code = _query(context, nativeName, nativeOptions, output);
      if (code != 0) {
        final detail = _text(_contextError(context));
        throw RatatoskrException(
            code, detail.isEmpty ? _text(_errorString(code)) : detail);
      }
      final result = output.value;
      final count = _count(result);
      if (count > 4096)
        throw const RatatoskrException(6, 'native result exceeds safety limit');
      final records = List<DnsRecord>.generate(count, (index) {
        final record = _record(result, index);
        return DnsRecord(
            typeCode: _recordType(record),
            section: _recordSection(record),
            name: _text(_recordName(record)),
            ttl: _recordTtl(record),
            text: _text(_recordText(record)));
      }, growable: false);
      return DnsResult(
          queryName: _text(_queryName(result)),
          queryType: _queryType(result),
          server: _text(_server(result)),
          responseCode: _rcode(result),
          records: records);
    } finally {
      if (output.value != nullptr) _destroyResult(output.value);
      _destroyContext(context);
      calloc.free(nativeOptions);
      calloc.free(output);
      calloc.free(nativeName);
      if (nativeServer != null) calloc.free(nativeServer);
    }
  }

  String _text(Pointer<Utf8> value) =>
      value == nullptr ? '' : value.toDartString();
  void _bind() {
    _abi = _library
        .lookupFunction<Uint32 Function(), int Function()>('ratos_abi_version');
    _create = _library.lookupFunction<Pointer<_Context> Function(),
        Pointer<_Context> Function()>('ratos_context_create');
    _destroyContext = _library.lookupFunction<Void Function(Pointer<_Context>),
        void Function(Pointer<_Context>)>('ratos_context_destroy');
    _contextError = _library.lookupFunction<
        Pointer<Utf8> Function(Pointer<_Context>),
        Pointer<Utf8> Function(Pointer<_Context>)>('ratos_context_error');
    _errorString = _library.lookupFunction<Pointer<Utf8> Function(Int32),
        Pointer<Utf8> Function(int)>('ratos_error_string');
    _init = _library.lookupFunction<Void Function(Pointer<_Options>),
        void Function(Pointer<_Options>)>('ratos_dns_query_options_init');
    _query = _library.lookupFunction<
        Int32 Function(Pointer<_Context>, Pointer<Utf8>, Pointer<_Options>,
            Pointer<Pointer<_Result>>),
        int Function(Pointer<_Context>, Pointer<Utf8>, Pointer<_Options>,
            Pointer<Pointer<_Result>>)>('ratos_dns_query');
    _destroyResult = _library.lookupFunction<Void Function(Pointer<_Result>),
        void Function(Pointer<_Result>)>('ratos_dns_result_destroy');
    _rcode = _library.lookupFunction<Uint8 Function(Pointer<_Result>),
        int Function(Pointer<_Result>)>('ratos_dns_result_rcode');
    _queryType = _library.lookupFunction<Uint16 Function(Pointer<_Result>),
        int Function(Pointer<_Result>)>('ratos_dns_result_query_type');
    _queryName = _library.lookupFunction<
        Pointer<Utf8> Function(Pointer<_Result>),
        Pointer<Utf8> Function(
            Pointer<_Result>)>('ratos_dns_result_query_name');
    _server = _library.lookupFunction<Pointer<Utf8> Function(Pointer<_Result>),
        Pointer<Utf8> Function(Pointer<_Result>)>('ratos_dns_result_server');
    _count = _library.lookupFunction<Size Function(Pointer<_Result>),
        int Function(Pointer<_Result>)>('ratos_dns_result_count');
    _record = _library.lookupFunction<
        Pointer<_Record> Function(Pointer<_Result>, Size),
        Pointer<_Record> Function(
            Pointer<_Result>, int)>('ratos_dns_result_record');
    _recordType = _library.lookupFunction<Uint16 Function(Pointer<_Record>),
        int Function(Pointer<_Record>)>('ratos_dns_record_type_code');
    _recordSection = _library.lookupFunction<Uint8 Function(Pointer<_Record>),
        int Function(Pointer<_Record>)>('ratos_dns_record_section');
    _recordTtl = _library.lookupFunction<Uint32 Function(Pointer<_Record>),
        int Function(Pointer<_Record>)>('ratos_dns_record_ttl');
    _recordName = _library.lookupFunction<
        Pointer<Utf8> Function(Pointer<_Record>),
        Pointer<Utf8> Function(Pointer<_Record>)>('ratos_dns_record_name');
    _recordText = _library.lookupFunction<
        Pointer<Utf8> Function(Pointer<_Record>),
        Pointer<Utf8> Function(Pointer<_Record>)>('ratos_dns_record_text');
  }

  static String _libraryPath() =>
      Platform.environment['RATATOSKR_LIBRARY'] ??
      (Platform.isWindows
          ? 'ratatoskr.dll'
          : Platform.isMacOS
              ? 'libratatoskr.dylib'
              : 'libratatoskr.so');
}
