// Package ratatoskr provides ownership-safe Go bindings to the native core.
package ratatoskr

/*
#cgo pkg-config: ratatoskr
#include <stdlib.h>
#include <ratatoskr/ratatoskr.h>
*/
import "C"

import (
	"errors"
	"fmt"
	"unsafe"
)

type RecordType uint16

const (
	A     RecordType = 1
	NS    RecordType = 2
	CNAME RecordType = 5
	SOA   RecordType = 6
	PTR   RecordType = 12
	MX    RecordType = 15
	TXT   RecordType = 16
	AAAA  RecordType = 28
	SRV   RecordType = 33
	NAPTR RecordType = 35
	CAA   RecordType = 257
)

type QueryOptions struct {
	Server           string
	Port             uint16
	TimeoutMS        uint32
	RecursionDesired *bool
}
type Record struct {
	TypeCode uint16
	Section  uint8
	Name     string
	TTL      uint32
	Text     string
}
type Result struct {
	QueryName    string
	QueryType    uint16
	Server       string
	ResponseCode uint8
	Records      []Record
}
type Error struct {
	Code    int
	Message string
}

func (e *Error) Error() string { return fmt.Sprintf("Ratatoskr error %d: %s", e.Code, e.Message) }

func ABI() uint32 { return uint32(C.ratos_abi_version()) }

func text(value *C.char) string {
	if value == nil {
		return ""
	}
	return C.GoString(value)
}

func Query(name string, recordType RecordType, options QueryOptions) (Result, error) {
	if name == "" {
		return Result{}, errors.New("name must not be empty")
	}
	if ABI() != 1 {
		return Result{}, errors.New("unsupported Ratatoskr native ABI")
	}
	nameValue := C.CString(name)
	defer C.free(unsafe.Pointer(nameValue))
	var serverValue *C.char
	if options.Server != "" {
		serverValue = C.CString(options.Server)
		defer C.free(unsafe.Pointer(serverValue))
	}
	context := C.ratos_context_create()
	if context == nil {
		return Result{}, errors.New("could not allocate native context")
	}
	defer C.ratos_context_destroy(context)
	var native C.ratos_dns_query_options
	C.ratos_dns_query_options_init(&native)
	native.server = serverValue
	if options.Port != 0 {
		native.port = C.uint16_t(options.Port)
	}
	native._type = C.ratos_dns_type(recordType)
	if options.TimeoutMS != 0 {
		native.timeout_ms = C.uint32_t(options.TimeoutMS)
	}
	if options.RecursionDesired != nil {
		if *options.RecursionDesired {
			native.recursion_desired = 1
		} else {
			native.recursion_desired = 0
		}
	}
	var result *C.ratos_dns_result
	code := C.ratos_dns_query(context, nameValue, &native, &result)
	if code != C.RATOS_OK {
		message := text(C.ratos_context_error(context))
		if message == "" {
			message = text(C.ratos_error_string(code))
		}
		return Result{}, &Error{Code: int(code), Message: message}
	}
	if result == nil {
		return Result{}, errors.New("native query returned no result")
	}
	defer C.ratos_dns_result_destroy(result)
	count := uint64(C.ratos_dns_result_count(result))
	if count > 4096 {
		return Result{}, errors.New("native result exceeds safety limit")
	}
	owned := Result{QueryName: text(C.ratos_dns_result_query_name(result)), QueryType: uint16(C.ratos_dns_result_query_type(result)), Server: text(C.ratos_dns_result_server(result)), ResponseCode: uint8(C.ratos_dns_result_rcode(result)), Records: make([]Record, 0, count)}
	for index := uint64(0); index < count; index++ {
		record := C.ratos_dns_result_record(result, C.size_t(index))
		if record == nil {
			continue
		}
		owned.Records = append(owned.Records, Record{TypeCode: uint16(C.ratos_dns_record_type_code(record)), Section: uint8(C.ratos_dns_record_section(record)), Name: text(C.ratos_dns_record_name(record)), TTL: uint32(C.ratos_dns_record_ttl(record)), Text: text(C.ratos_dns_record_text(record))})
	}
	return owned, nil
}
