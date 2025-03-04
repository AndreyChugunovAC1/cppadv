#include <iostream>
#include <string>
#include <vector>

int main() {
  std::string s        =   "abc 😦"; // utf 8 will be written by char
  std::u32string s_cp  =  U"abc 😦";
  // std::u8string s_utf8 = u8"abc 😦";

  std::cout << s << "\n";
  // std::cout << s_cp << "\n";
  // std::cout << s_utf8 << "\n";
}