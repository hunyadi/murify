/**
 * murify: Efficient in-memory compression for URLs
 * @see https://github.com/hunyadi/murify
 *
 * Copyright (c) 2024 Levente Hunyadi
 *
 * This work is licensed under the terms of the MIT license.
 * For a copy, see <https://opensource.org/licenses/MIT>.
 */

#include <murify/compactor.hpp>
#include <murify/base64url.hpp>
#include <array>
#include <iostream>

template<typename Compactor>
static void check(Compactor& c, const std::string_view& ref)
{
    auto enc = c.compact(std::string_view(ref));
    auto dec = c.expand(enc);
    if (dec != ref) {
        std::array<char, 256> buf;
        int n = std::snprintf(buf.data(), buf.size(), "expected: %s; got: %s", ref.data(), dec.data());
        throw std::runtime_error(std::string(buf.data(), n));
    }
    if (ref.size() > 0) {
        std::cout << "saved " << (100 - static_cast<int>(100 * enc.size() / ref.size())) << "% on " << ref << std::endl;
    }
}

static void check_encode(const std::string_view& str, const std::string_view& ref)
{
    std::basic_string<std::byte> in;
    in.resize(str.size());
    std::memcpy(in.data(), str.data(), str.size());
    std::string enc;
    if (!murify::base64::encode(in, enc)) {
        std::array<char, 256> buf;
        std::size_t n = std::snprintf(buf.data(), buf.size(), "expected: %s; got encode error", ref.data());
        throw std::runtime_error(std::string(buf.data(), buf.data() + n));
    }
    if (enc != ref) {
        std::array<char, 256> buf;
        std::size_t n = std::snprintf(buf.data(), buf.size(), "expected: %s; got: %s", ref.data(), enc.data());
        throw std::runtime_error(std::string(buf.data(), buf.data() + n));
    }
    std::basic_string<std::byte> dec;
    if (!murify::base64::decode(ref, dec)) {
        std::array<char, 256> buf;
        std::size_t n = std::snprintf(buf.data(), buf.size(), "expected: %s; got decode error", ref.data());
        throw std::runtime_error(std::string(buf.data(), buf.data() + n));
    }
    std::string out;
    out.resize(dec.size());
    std::memcpy(out.data(), dec.data(), dec.size());
    if (out != str) {
        std::array<char, 256> buf;
        int n = std::snprintf(buf.data(), buf.size(), "expected: %s; got: %s", str.data(), out.data());
        throw std::runtime_error(std::string(buf.data(), buf.data() + n));
    }
}

int main(int /*argc*/, char* /*argv*/[])
{
    check_encode("", "");
    check_encode("f", "Zg");
    check_encode("fo", "Zm8");
    check_encode("foo", "Zm9v");
    check_encode("foob", "Zm9vYg");
    check_encode("fooba", "Zm9vYmE");
    check_encode("foobar", "Zm9vYmFy");
    check_encode("extended-academic-research", "ZXh0ZW5kZWQtYWNhZGVtaWMtcmVzZWFyY2g");

    murify::PathCompactor pc;
    check(pc, std::string_view());
    check(pc, "");
    check(pc, "0");
    check(pc, "123");
    check(pc, "4294967295");
    check(pc, "18446744073709551615");
    check(pc, "alma");
    check(pc, "extended-academic-research");  // uses Base64 encoding
    check(pc, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    check(pc, "/");
    check(pc, "1/");
    check(pc, "/2");
    check(pc, "///");
    check(pc, "0/1/2/3");
    check(pc, "a/b/c/d");
    check(pc, "a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t/u/v/w/x/y/z/A/B/C/D/E/F/G/H/I/J/K/L/M/N/O/P/Q/R/S/T/U/V/W/X/Y/Z");
    check(pc, "aa/bb/cc/dd/ee/ff/gg/hh/ii/jj/kk/ll/mm/nn/oo/pp/qq/rr/ss/tt/uu/vv/ww/xx/yy/zz/AA/BB/CC/DD/EE/FF/GG/HH/II/JJ/KK/LL/MM/NN/OO/PP/QQ/RR/SS/TT/UU/VV/WW/XX/YY/ZZ");

    murify::QueryCompactor qc;
    check(qc, std::string_view());
    check(qc, "");
    check(qc, "value");
    check(qc, "key=0");
    check(qc, "key=4294967295");
    check(qc, "key=18446744073709551615");
    check(qc, "number=0&string=alma");
    check(qc, "&&");
    check(qc, "&key=&");
    check(qc, "auth=eyJh..eyJh");
    check(qc, "auth=eyJh.abc.eyJh");
    check(qc, "auth=eyJh.@bc.eyJh");
    check(qc, "auth=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c");
    check(qc, "sig=SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c");

    std::string str;
    str.append("action=test");
    for (std::size_t k = 0; k < 500; ++k) {
        str.append("&");
        str.append("value=");
        str.append(std::to_string(k));
    }
    check(qc, str);

    murify::URLCompactor uc;
    check(uc, "g:h");
    check(uc, "http://a/b/c/g");
    check(uc, "http://a/b/c/g");
    check(uc, "http://a/b/c/g/");
    check(uc, "http://a/g");
    check(uc, "http://g");
    check(uc, "http://a/b/c/?y");
    check(uc, "http://a/b/c/g?y");
    check(uc, "http://a/b/c/d;p?q#s");
    check(uc, "http://a/b/c/g#s");
    check(uc, "http://a/b/c/g?y#s");
    check(uc, "http://a/b/c/;x");
    check(uc, "http://a/b/c/g;x");
    check(uc, "http://a/b/c/g;x?y#s");
    check(uc, "http://a/b/c/");
    check(uc, "http://a/b/c/");
    check(uc, "http://a/b/");
    check(uc, "http://a/b/");
    check(uc, "http://a/b/g");
    check(uc, "http://a/");
    check(uc, "http://a/");
    check(uc, "http://a/g");
    check(uc, "http://a/../g");
    check(uc, "http://a/../../g");
    check(uc, "http://a/./g");
    check(uc, "http://a/../g");
    check(uc, "http://a/b/c/g.");
    check(uc, "http://a/b/c/.g");
    check(uc, "http://a/b/c/g..");
    check(uc, "http://a/b/c/..g");
    check(uc, "http://a/b/g");
    check(uc, "http://a/b/c/g/");
    check(uc, "http://a/b/c/g/h");
    check(uc, "http://a/b/c/h");
    check(uc, "http://a/b/c/g;x=1/y");
    check(uc, "http://a/b/c/y");
    check(uc, "http://a/b/c/g?y/./x");
    check(uc, "http://a/b/c/g?y/../x");
    check(uc, "http://a/b/c/g#s/./x");
    check(uc, "http://a/b/c/g#s/../x");

    return 0;
}
