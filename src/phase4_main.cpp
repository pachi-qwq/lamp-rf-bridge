// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <cstring>
#include "config.h"
#include "lamp_controller.h"
#include "network_app.h"

namespace {
lamp::Controller controller;
char line[80]{};
std::size_t used = 0;
bool tooLong = false;
void serialCommand() {
  if (std::strcmp(line, "access") == 0)
    network_app::printAccess();
  else if (std::strcmp(line, "setup") == 0)
    network_app::enableSetup();
  else if (std::strcmp(line, "status") == 0) {
    controller.healthy();
    Serial.printf(
        "{\"type\":\"status\",\"phase\":4,\"ready\":%s,\"marcstate\":%u,"
        "\"bus_error\":%d,\"attempts\":%lu}\n",
        controller.ready() ? "true" : "false", controller.marcstate(), controller.busError(),
        static_cast<unsigned long>(controller.attempts()));
  } else
    Serial.println("{\"type\":\"error\",\"message\":\"unknown_command\"}");
}
}  // namespace
void setup() {
  Serial.begin(config::kSerialBaud);
  delay(300);
  controller.begin();
  if (!network_app::begin(controller)) Serial.println("{\"type\":\"network_setup_failed\"}");
}

void loop() {
  network_app::loop();
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      if (!tooLong && used) {
        line[used] = 0;
        serialCommand();
      }
      used = 0;
      tooLong = false;
    } else if (used < sizeof(line) - 1)
      line[used++] = c;
    else
      tooLong = true;
  }
  delay(2);
}
