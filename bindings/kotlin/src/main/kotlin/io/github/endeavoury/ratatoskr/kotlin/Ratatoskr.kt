package io.github.endeavoury.ratatoskr.kotlin

import io.github.endeavoury.ratatoskr.DnsClient as JavaDnsClient
import io.github.endeavoury.ratatoskr.DnsQueryOptions as JavaDnsQueryOptions
import io.github.endeavoury.ratatoskr.DnsRecord as JavaDnsRecord
import io.github.endeavoury.ratatoskr.DnsRecordType
import io.github.endeavoury.ratatoskr.DnsResult as JavaDnsResult
import io.github.endeavoury.ratatoskr.Ratatoskr as JavaRatatoskr

typealias DnsRecord = JavaDnsRecord
typealias DnsResult = JavaDnsResult
typealias DnsQueryOptions = JavaDnsQueryOptions

object Ratatoskr {
    val abiVersion: Int get() = JavaRatatoskr.abiVersion()
    val version: String get() = JavaRatatoskr.version()
    fun dns(options: DnsQueryOptions = DnsQueryOptions.defaults()) = DnsClient(options)
}

class DnsClient(options: DnsQueryOptions = DnsQueryOptions.defaults()) {
    private val delegate = JavaDnsClient(options)
    fun query(name: String, type: DnsRecordType = DnsRecordType.A): DnsResult = delegate.query(name, type)
}
