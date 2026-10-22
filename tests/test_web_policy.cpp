// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <iostream>
#include <string>
#include "web_policy.h"
int main() {
  unsigned count = 0;
  const auto check = [&count](bool pass) {
    if (!pass) std::exit(1);
    ++count;
  };
  check(web_policy::validRequestId("0123456789abcdef"));
  check(!web_policy::validRequestId("0123456789abcde"));
  check(!web_policy::validRequestId("0123456789ABCDEF"));
  check(!web_policy::validRequestId("0123456789abcde!"));
  check(!web_policy::validRequestId(std::string(49, 'a').c_str()));
  check(web_policy::validWifi("Home", "12345678"));
  check(web_policy::validWifi("Home", ""));
  check(web_policy::validWifi("家庭网络", "some password"));
  check(!web_policy::validWifi("", "12345678"));
  check(!web_policy::validWifi("Home", "1234567"));
  check(!web_policy::validWifi("Home", std::string(64, 'a').c_str()));
  check(!web_policy::validWifi(std::string(33, 'a').c_str(), "12345678"));
  check(!web_policy::validWifi("Home\nNetwork", "12345678"));
  check(!web_policy::validWifi("Home", "12345678\n"));
  check(!web_policy::nonceExpired(100, 3600100));
  check(web_policy::nonceExpired(100, 3600101));
  check(!web_policy::nonceExpired(0xFFFFFF00U, 0x000000FFU));

  // Real elapsed-time boundaries, including a previously expired token after millis() wrap.
  // 真实时间差边界，包含 millis() 回绕后不应复活的旧凭证。
  check(web_policy::sessionActive(100, 100));
  check(web_policy::sessionActive(100, 28800099ULL));
  check(!web_policy::sessionActive(100, 28800100ULL));
  check(!web_policy::sessionActive(100, 99));
  check(!web_policy::sessionActive(100, (1ULL << 32) + 200));
  check(web_policy::sessionActive((1ULL << 32) - 100, (1ULL << 32) + 100));
  check(web_policy::authenticationEpoch(0) == 0);
  check(web_policy::authenticationEpoch((1ULL << 32) - 1) == 0);
  check(web_policy::authenticationEpoch(1ULL << 32) == 1);
  check(web_policy::authenticationEpoch((2ULL << 32) + 123) == 2);

  const std::string token(64, 'a');
  char parsed[65]{};
  check(web_policy::validTrustToken(token.c_str()));
  check(!web_policy::validTrustToken(nullptr));
  check(!web_policy::validTrustToken(std::string(63, 'a').c_str()));
  check(!web_policy::validTrustToken(std::string(65, 'a').c_str()));
  check(!web_policy::validTrustToken(std::string(64, 'A').c_str()));
  check(!web_policy::validTrustToken((std::string(63, 'a') + "!").c_str()));
  check(web_policy::readTrustCookie(("lamp_device=" + token).c_str(), parsed) && parsed == token);
  check(web_policy::readTrustCookie(("foo=bar; lamp_device=" + token + "; other=1").c_str(),
                                    parsed) &&
        parsed == token);
  check(web_policy::readTrustCookie(("\t lamp_device=" + token).c_str(), parsed));
  check(!web_policy::readTrustCookie("", parsed));
  check(!web_policy::readTrustCookie(nullptr, parsed));
  check(!web_policy::readTrustCookie(("not_lamp_device=" + token).c_str(), parsed));
  check(!web_policy::readTrustCookie(("lamp_device=" + token + "a").c_str(), parsed));
  check(!web_policy::readTrustCookie(("lamp_device=" + token + "; lamp_device=" + token).c_str(),
                                     parsed));
  check(!web_policy::readTrustCookie(("lamp_device=" + token + "; lamp_device=").c_str(), parsed));
  check(!web_policy::readTrustCookie(("lamp_device=\"" + token + "\"").c_str(), parsed));
  std::cout << "PASS: " << count << " web policy checks (no hardware simulation)\n";
}
