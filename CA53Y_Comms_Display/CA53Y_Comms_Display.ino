/*
  CA53Y Comms Display
  ESP-NOW mesh messenger for the 2.8" ESP32-32E touch display (E32R28T / E32R28T-1, 240x320, resistive touch).

  - Direct messages to contacts, open or encrypted with a shared PIN (AES-128-GCM).
  - Channels (group chats): open, or private with a passphrase.
  - Repeats every CA53Y message it hears for other people (mesh relay, up to 8 hops).
  - Nearby list of CA53Y devices in direct range, with signal strength.
  - Settings > Power off / Restart. Press BOOT to turn it back on.
  - Talks to the CA53Y Comms dev-module messengers and relay boards (protocol v2, channel 1).

  Build: Arduino-ESP32 core 3.x, board "ESP32 Dev Module", library LovyanGFX.
  Pick your screen version in config.h (CA53Y_PANEL). Full instructions: README.md.
*/

#include "config.h"
#include "board.h"
#include "net.h"
#include "power.h"
#include "ui.h"

void finishSetup() {
  isSetup = true;
  prefs.putBool("setup", true);
  if (nChans == 0) { addChannel("Public", ""); saveChannels(); }
  scr = SCR_HOME;
  tab = TAB_CHATS;
  needDraw = true;
}

void setup() {
  powerBegin();       // undo the pin holds from Power off
  Serial.begin(115200);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  loadState();
  uiSetup();          // screen, touch (first boot: calibration), splash
  netSetup();         // ESP-NOW on channel 1
  delay(1600);        // let the splash show
  if (!isSetup) openEditor(ED_MYNAME, "Your name", "", 23, false, "Shown to people you message. You can change it later in Settings.");
  else scr = SCR_HOME;
  needDraw = true;
}

void loop() {
  netLoop();
  uiLoop();

  // hold BOOT for 5 seconds to erase everything (keeps the device ID)
  static uint32_t pressStart = 0;
  if (digitalRead(PIN_BOOT) == LOW) {
    uint32_t now = millis();
    if (!pressStart) pressStart = now;
    if (now - pressStart > 1500) digitalWrite(PIN_LED_R, (now / 100) & 1);
    if (now - pressStart > 5000) {
      String id = prefs.getString("id", "");
      prefs.clear();
      prefs.putString("id", id);
      ESP.restart();
    }
  } else if (pressStart) {
    pressStart = 0;
    digitalWrite(PIN_LED_R, HIGH);
  }
  delay(2);
}
