# Ratatoskr for PHP

Install with Composer/Packagist as `ratatoskr/ratatoskr`. PHP's FFI extension
must be enabled, and `libratatoskr` must be installed or selected with
`RATATOSKR_LIBRARY`.

```php
$result = Ratatoskr\Dns::query('example.com', Ratatoskr\DnsRecordType::A);
```
