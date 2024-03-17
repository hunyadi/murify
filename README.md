# murify: Efficient in-memory compression for URLs

This header-only C++ library converts URLs into a fully reversible compact representation. While general-purpose compression algorithms (e.g. GZIP or ZSTD) may not operate efficiently on URLs, URL compaction may reduce URL string length by as much as 75%, at low CPU cost. This may save substantial space when millions of URLs have to be kept in memory simultaneously.

Compaction is accomplished with a combination of several techniques:

* Decimal integers are represented as their binary equivalent, packed into minimum width. For example, the character string `123` of length 3 becomes the hexadecimal value `0x7B` and is persisted in a single byte. The character string `4294967295` of length 10 becomes the hexadecimal value `0xFFFFFFFF` and is persisted in 4 bytes.
* Frequently occurring strings (such as components in a path, or keys in a query string) are interned, and only the index in the lookup table is stored, packed into minimum width. A long but frequent path component such as `management` may become an index stored in a single byte.
* UUID strings (typically 36 characters) are parsed into a 16-byte array.
* When Base64-encoded data is encountered (e.g. a JWT or a user identifier), it's decoded and the raw representation is persisted, resulting in savings of 25%.
* Type is identified with a control byte. Integer width, string length or lookup table index is packed into the control byte whenever possible.
* Composite types such as URL path or query string are persisted as a combination of length and series of values, separators (e.g. `/`, `&` or `=`) are not stored.
