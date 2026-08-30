"""Query an A record with the Ratatoskr native core."""

import ratatoskr

result = ratatoskr.dns.query("example.com", record_type="A")
for answer in result.answers:
    print(answer.text)
