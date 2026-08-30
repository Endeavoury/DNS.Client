# DNS fuzz seed corpus

The auditable hex seed descriptions cover valid A/AAAA/MX/TXT/PTR packets,
compression, truncation, pointer errors, invalid RDLENGTH, empty input, and maximum
packet boundaries. Release fuzz jobs decode each line after its label into a binary
libFuzzer corpus directory; generated binary corpus files are build artifacts.
