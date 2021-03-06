#pragma once

#include "esphome.h"
// **********************************************************************************
#include <RFM69_ATC.h>
#include <SPI.h> // Not actually used but needed to compile
#include "Payload.h"
#include "PayloadStorage.h"
#include "Settings.h"
#include "Credentials.h"
#include <functional>

class RFM69Manager : public Component {
 public:

  RFM69Manager(/*PayloadStorage *payload_storage*/) : _radio(_SPI_CS, _IRQ_PIN) {
    //this->_payload_storage = payload_storage;
  }

  void setMqttSender(std::function<void(const Payload&)> ptr){
    this->mqttsender = ptr;
  }

  void setup() override {
    ESP_LOGI(__FUNCTION__, "exec...");

    ESP_LOGI(__FUNCTION__, "Initializing RFM69HW radio: Frequency: [868MHZ], NodeId: [%d], NetworkId: [%d]",
      NODEID, NETWORKID);
    _radio.initialize(FREQUENCY,NODEID,NETWORKID);
    _radio.setHighPower(); //must include this only for RFM69HW/HCW!
    _radio.encrypt(ENCRYPTKEY);
    _radio.spyMode(false);
    ESP_LOGI(__FUNCTION__, "RFM69_ATC Enabled (Auto Transmission Control)");
  }
  void loop() override {
    receive();
    //send();
  }

  void receive(){
    while (_radio.receiveDone())
    {
      Payload payload(reinterpret_cast<char*>(_radio.DATA), _radio.SENDERID, _radio.RSSI);
      _packetCount++;

      ESP_LOGI(__FUNCTION__, "Received: [%d][%d][%d][RX_RSSI:%d] - [%s]", 
        _packetCount, 
        payload.get_device_id(), 
        _radio.DATALEN, 
        payload.get_rssi(), 
        payload.get_raw().c_str()
      );

      mqttsender(payload);

      // Append payload to queue
      //_payload_storage->get_incoming_messages_queue().push(payload);

      if (_radio.ACKRequested())
      {
        _radio.sendACK();
        ESP_LOGI(__FUNCTION__, "ACK sent");
      }

      blink();
    }
  }

  void send(const Payload &p){
    // Check if new message in queue
    //if(!_payload_storage->get_outcoming_messages_queue().empty()){
      //ayload p = _payload_storage->get_outcoming_messages_queue().pop();

      ESP_LOGD(__FUNCTION__, "Sending payload=[%s] to targed id=[%d]", p.get_raw().c_str(), p.get_device_id());
      if(_radio.sendWithRetry(p.get_device_id(), p.get_raw().c_str(), 8)){
        ESP_LOGI(__FUNCTION__, "Sent OK!");
      } else {
        ESP_LOGW(__FUNCTION__, "Sent FAILED!");
      }

      blink();
    //}
  }

  void blink()
  {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN,HIGH);
    delay(3);
    digitalWrite(LED_BUILTIN,LOW);
  }

private:
  //Auto Transmission Control - dials down transmit power to save battery
  //Usually you do not need to always transmit at max output power
  //By reducing TX power even a little you save a significant amount of battery power
  //This setting enables this gateway to work with remote nodes that have ATC enabled to
  //dial their power down to only the required level
  RFM69_ATC _radio;

  uint32_t _packetCount = 0;

  static const constexpr int _SPI_CS = RF69_SPI_CS;
  static const constexpr int _IRQ_PIN = 4;
  
  std::function<void(const Payload&)> mqttsender = nullptr;
  //PayloadStorage *_payload_storage = nullptr;
};

