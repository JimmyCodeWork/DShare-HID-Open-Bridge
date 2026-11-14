// Copyright (C) 2026 DShare-HID contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <Arduino.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <esp_system.h>
#include <mbedtls/pk.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/sha256.h>
#include "bridge_identity.h"
#include "usb_frame_decoder.h"
#include "hid_state.h"
#include <cstring>

using namespace open_bridge;
namespace {
// Three report IDs: boot-style keyboard, 16-bit relative mouse, consumer control.
uint8_t reportMap[] = {
  0x05,0x01,0x09,0x06,0xa1,0x01,0x85,0x01,0x05,0x07,0x19,0xe0,0x29,0xe7,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,
 0x95,0x01,0x75,0x08,0x81,0x01,0x95,0x06,0x75,0x08,0x15,0x00,0x26,0xff,0x00,0x05,0x07,0x19,0x00,0x2a,0xff,0x00,0x81,0x00,0xc0,
 0x05,0x01,0x09,0x02,0xa1,0x01,0x85,0x02,0x09,0x01,0xa1,0x00,0x05,0x09,0x19,0x01,0x29,0x08,0x15,0x00,0x25,0x01,0x95,0x08,0x75,0x01,0x81,0x02,
 0x05,0x01,0x09,0x30,0x09,0x31,0x16,0x01,0x80,0x26,0xff,0x7f,0x75,0x10,0x95,0x02,0x81,0x06,
 0x09,0x38,0x15,0x81,0x25,0x7f,0x75,0x08,0x95,0x01,0x81,0x06,0xc0,0xc0,
 0x05,0x0c,0x09,0x01,0xa1,0x01,0x85,0x03,0x15,0x00,0x26,0xff,0x03,0x19,0x00,0x2a,0xff,0x03,0x75,0x10,0x95,0x01,0x81,0x00,0xc0
};
Preferences settings;
NimBLEServer *server;
NimBLECharacteristic *reports[3]{};
volatile bool bleDisconnected = false;
bool authenticated = false;
bool idleReleased = false;
uint32_t lastFrame = 0, lastByte = 0, lastMotion = 0;
// Send accumulated mouse movement at most this often, close to the Bluetooth
// HID connection interval, so reports never back up (see hid_state.h).
constexpr uint32_t kMotionIntervalMs = 10;
uint8_t profile[52]{};
String deviceName;

void sendReport(void *, HidState::Report type, const uint8_t *data, size_t len) {
  const unsigned i = static_cast<unsigned>(type);
  if (server && server->getConnectedCount() && reports[i]) reports[i]->notify(data, len);
}
HidState hid(sendReport, nullptr);
class Callbacks : public NimBLEServerCallbacks {
  void onDisconnect(NimBLEServer *) override { bleDisconnected = true; NimBLEDevice::startAdvertising(); }
  void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
    if (!desc->sec_state.encrypted) server->disconnect(desc->conn_handle);
  }
} callbacks;

void frame(const uint8_t *data, size_t n) {
  uint8_t header[] = {0x5a,0xc3,1,0x80,0,0,static_cast<uint8_t>(n),static_cast<uint8_t>(n >> 8)};
  Serial.write(header, sizeof(header)); Serial.write(data, n); Serial.flush();
}
void config(uint8_t command, uint8_t status, const uint8_t *data = nullptr, size_t len = 0) {
  uint8_t response[3 + 52] = {0x82, command, status};
  if (data && len <= 52) memcpy(response + 3, data, len); else len = 0;
  frame(response, 3 + len);
}
void ack(uint8_t status, const uint8_t *data = nullptr, size_t len = 0) {
  uint8_t response[2 + 52] = {0x81,status};
  if (data && len <= 52) memcpy(response + 2,data,len); else len = 0;
  frame(response, 2 + len);
}
int rng(void *, unsigned char *out, size_t len) { esp_fill_random(out, len); return 0; }
bool signNonce(const uint8_t *host, uint8_t *nonce, uint8_t *sig) {
  esp_fill_random(nonce, 32);
  uint8_t message[64], digest[32]; memcpy(message, host, 32); memcpy(message + 32, nonce, 32);
  mbedtls_sha256_ret(message, sizeof(message), digest, 0);
  mbedtls_pk_context pk; mbedtls_pk_init(&pk);
  mbedtls_mpi r,s; mbedtls_mpi_init(&r); mbedtls_mpi_init(&s);
  bool ok = mbedtls_pk_parse_key(&pk, bridge_private_key, sizeof(bridge_private_key), nullptr, 0) == 0 &&
            mbedtls_pk_can_do(&pk, MBEDTLS_PK_ECKEY) &&
            mbedtls_ecdsa_sign(&mbedtls_pk_ec(pk)->grp, &r, &s, &mbedtls_pk_ec(pk)->d,
                               digest, sizeof(digest), rng, nullptr) == 0 &&
            mbedtls_mpi_write_binary(&r, sig, 32) == 0 && mbedtls_mpi_write_binary(&s, sig + 32, 32) == 0;
  mbedtls_mpi_free(&r); mbedtls_mpi_free(&s); mbedtls_pk_free(&pk); return ok;
}
void resetProfile() {
  memset(profile,0,sizeof(profile)); memcpy(profile,"iPad",4);
  profile[34]=0x38; profile[35]=0x04; // 1080
  profile[36]=0x78; profile[37]=0x09; // 2424
  profile[40]=30; profile[41]=10;
}
void handle(void *, uint8_t type, uint8_t, const uint8_t *p, size_t n) {
  lastFrame = millis();
  idleReleased = false;
  if (type == 1) { if (authenticated) hid.handle(p,n); return; }
  if (type != 0x80 || !n) return;
  const uint8_t cmd = p[0];
  if (cmd == 1) {
    authenticated = false; hid.releaseAll();
    if (n != 99 || p[1] != 1 || p[2] != 2) return;
    uint8_t response[113] = {0x81,1,3,1,10,1,0,0x10,0,0,5};
    response[8] = server->getConnectedCount() ? 1 : 0;
    if (!signNonce(p+3,response+17,response+49)) return;
    authenticated = true; frame(response,sizeof(response)); return;
  }
  if (!authenticated) return;
  switch (cmd) {
  case 2: config(cmd,0,reinterpret_cast<const uint8_t *>(deviceName.c_str()),deviceName.length()); break;
  case 3:
    if (n < 2 || n > 23 || memchr(p+1,0,n-1)) { config(cmd,1); break; }
    deviceName = String(); for (size_t i=1;i<n;i++) deviceName += char(p[i]);
    config(cmd, settings.putString("name",deviceName) ? 0 : 1); break;
  case 4: {
    char serial[24]; snprintf(serial,sizeof(serial),"Open-%012llX",ESP.getEfuseMac());
    config(cmd,0,reinterpret_cast<uint8_t *>(serial),strlen(serial)); break;
  }
  case 9: config(cmd,0); break;
  case 0x30: NimBLEDevice::deleteAllBonds(); hid.releaseAll(); ack(0); break;
  case 0x31: ack(n == 2 && p[1] == 0 ? 0 : 1); break;
  case 0x32: if (n == 1 || (n == 2 && p[1] == 0)) ack(0,profile,sizeof(profile)); else ack(1); break;
  case 0x33:
    if (n != 54 || p[1] != 0) { ack(1); break; }
    memcpy(profile,p+2,sizeof(profile)); profile[31]=0; profile[32]=0;
    ack(settings.putBytes("profile",profile,sizeof(profile)) == sizeof(profile) ? 0 : 1); break;
  case 0x34: case 0x35:
    if ((cmd == 0x34 && (n != 2 || p[1] != 0)) || (cmd == 0x35 && n != 1)) { ack(1); break; }
    resetProfile(); ack(settings.putBytes("profile",profile,sizeof(profile)) == sizeof(profile) ? 0 : 1); break;
  default: config(cmd,1); break;
  }
}
UsbFrameDecoder decoder(handle,nullptr);
}
void setup() {
  Serial.begin(115200);
  settings.begin("open-bridge",false);
  deviceName = settings.getString("name","Open Bridge");
  resetProfile(); if (settings.getBytesLength("profile") == sizeof(profile)) settings.getBytes("profile",profile,sizeof(profile));
  profile[31]=0; profile[32]=0;
  NimBLEDevice::init(deviceName.c_str()); NimBLEDevice::setSecurityAuth(true,false,true);
  server = NimBLEDevice::createServer(); server->setCallbacks(&callbacks);
  auto *bleHid = new NimBLEHIDDevice(server);
  reports[0]=bleHid->inputReport(1); reports[1]=bleHid->inputReport(2); reports[2]=bleHid->inputReport(3);
  bleHid->outputReport(1); bleHid->manufacturer("Open Bridge"); bleHid->pnp(2,0x303a,0x1001,0x0100);
  bleHid->hidInfo(0,1); bleHid->reportMap(reportMap,sizeof(reportMap)); bleHid->startServices(); bleHid->setBatteryLevel(100);
  auto *advertising = NimBLEDevice::getAdvertising(); advertising->setAppearance(0x03c1);
  advertising->addServiceUUID(bleHid->hidService()->getUUID()); advertising->start();
}
void loop() {
  if (bleDisconnected) { bleDisconnected=false; hid.releaseAll(); }
  if (authenticated && !idleReleased && uint32_t(millis()-lastFrame)>90000) { hid.releaseAll(); idleReleased=true; }
  if (uint32_t(millis()-lastByte)>2000) decoder.reset();
  while (Serial.available()) { int c=Serial.read(); if (c>=0) { uint8_t b=c; decoder.feed(&b,1); lastByte=millis(); } }
  if (hid.hasPendingMotion() && uint32_t(millis()-lastMotion) >= kMotionIntervalMs) { hid.flushMotion(); lastMotion=millis(); }
  delay(1);
}
