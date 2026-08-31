<?php
declare(strict_types=1);

namespace Ratatoskr;

enum DnsRecordType: int
{
    case A = 1; case NS = 2; case CNAME = 5; case SOA = 6; case PTR = 12;
    case MX = 15; case TXT = 16; case AAAA = 28; case SRV = 33;
    case NAPTR = 35; case CAA = 257;
}
