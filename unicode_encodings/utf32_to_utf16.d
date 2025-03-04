import core.stdc.stdint;

int16_t[] utf32_to_utf16(int32_t[] codePoints) {
  int16_t[] res;

  foreach (cp; codePoints) {
    if (cp >= 0x110000)
      throw new Exception("incorrect code point");
    // restricted [0xD800..0xE000)
    if (cp >= 0xD800 && cp < 0xE000)
      throw new Exception("incorrect code point");

    if (cp < 0x10000)
      res ~= cp;
    else {
      cp -= 0x10000;
      res ~= 0xD800 | cast(int16_t)(cp >> 10); //   [D800..DC00)
      res ~= 0xDC00 | cast(int16_t)(cp & 0x3FF); // [DC00..E000)
    }
  }
}

alias cp_to_utf16 = utf32_to_utf16;

int main() {
  string utf8 = "å ø ∑ 😦";
  wstring utf16 = "å ø ∑ 😦";
  dstring utf32 = "å ø ∑ 😦";
}
