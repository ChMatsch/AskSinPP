//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "AskSinPP.h"
#include "Sign.h"
#include "HMID.h"
#include "Channel.h"
#include "ChannelList.h"
#include "Message.h"
#include "Radio.h"
#include "Led.h"
#include "Buzzer.h"
#include "Activity.h"

#ifdef USE_HW_SERIAL
  #if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328PB__)
    #include <avr/boot.h>
#elif defined (ARDUINO_ARCH_STM32F1)
#elif defined (ARDUINO_ARCH_STM32) && defined (STM32L1xx)
#elif defined (ARDUINO_ARCH_ESP32)
#elif defined (ARDUINO_ARCH_RP2040)
#else
    #error Using Hardware serial is not supported on MCU type currently used
  #endif
#endif

#ifdef ARDUINO_ARCH_ESP32
uint8_t boot_signature_byte_get(byte addr) {
  uint8_t idByteLen = 6;
  uint64_t chipId = ESP.getEfuseMac();
  uint8_t *chipIdArray = *reinterpret_cast<uint8_t(*)[sizeof(uint64_t)]>(&chipId);
  std::reverse(&chipIdArray[0], &chipIdArray[idByteLen]);
  //DPRINT("ESP32 ChipID is ");for (uint8_t i = 0; i < idByteLen; i++) DHEX(chipIdArray[i]);DPRINTLN("");

  //we have only 6 bytes for unique identification
  //so let's reuse 4 bytes for the missing
  byte idx = 0;
  switch (addr) {
    case 14:
    case 20:
      idx = 0;
    break;
    case 15:
    case 21:
      idx = 1;
    break;
    case 16:
    case 22:
      idx = 2;
    break;
    case 17:
    case 23:
      idx = 3;
    break;
    case 18:
      idx = 4;
    break;
    case 19:
      idx = 5;
    break;

  }

  return chipIdArray[idx];
}
#endif

#ifdef ARDUINO_ARCH_RP2040
#include "pico/unique_id.h"
uint8_t boot_signature_byte_get(byte addr) {
  pico_unique_board_id_t id_out;
  pico_get_unique_board_id(&id_out);

  byte idx = 0;
  switch (addr) {
    case 14:
    case 20:
      idx = 0;
    break;
    case 15:
    case 21:
      idx = 1;
    break;
    case 16:
    case 22:
      idx = 2;
    break;
    case 17:
    case 23:
      idx = 3;
    break;
    case 18:
      idx = 4;
    break;
    case 19:
      idx = 5;
    break;
  }

  return id_out.id[idx];
}
#endif

#if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
#define OTA_CONFIG_START 0xffe0 // start address of 16 byte config data in bootloader
#define OTA_MODEL_START  0xfff0 // start address of 2 byte model id in bootloader
#define OTA_SERIAL_START 0xfff2 // start address of 10 byte serial number in bootloader
#define OTA_HMID_START   0xfffc // start address of 3 byte device id in bootloader
#else
#define OTA_CONFIG_START 0x7fe0 // start address of 16 byte config data in bootloader
#define OTA_MODEL_START  0x7ff0 // start address of 2 byte model id in bootloader
#define OTA_SERIAL_START 0x7ff2 // start address of 10 byte serial number in bootloader
#define OTA_HMID_START   0x7ffc // start address of 3 byte device id in bootloader
#endif

namespace as {

class DeviceType {
public:
enum Types {
  AlarmControl = 0x01,
  Switch = 0x10,
  OutputUnit = 0x12,
  Dimmer = 0x20,
  BlindActuator = 0x30,
  ClimateControl = 0x39,
  Remote = 0x40,
  Sensor = 0x41,
  Swi = 0x42,
  PushButton = 0x43,
  SingleButton = 0x44,
  PowerMeter = 0x51,
  Thermostat = 0x58,
  KFM100 = 0x60,
  THSensor = 0x70,
  ThreeStateSensor = 0x80,
  MotionDetector = 0x81,
  KeyMatic = 0xC0,
  WinMatic = 0xC1,
  TipTronic = 0xC3,
  SmokeDetector = 0xCD,
};
};

struct DeviceInfo {
  uint8_t DeviceID[3];
  char    Serial[11];
  uint8_t DeviceModel[2];
  uint8_t Firmware;
  uint8_t DeviceType;
  uint8_t DeviceInfo[2];
};

template <class HalType,class List0Type=List0>
class Device {

public:
  typedef typename HalType::LedType LedType;
  typedef typename HalType::BatteryType BatteryType;
  typedef typename HalType::RadioType RadioType;
  typedef typename HalType::BuzzerType BuzzerType;
  typedef List0Type List0;

private:
  HalType* hal;
  List0Type&   list0;
  uint8_t  msgcount;

  HMID    lastdev;
  uint8_t lastmsg;

#ifdef USE_HW_SERIAL
   uint8_t device_id[3];
#endif

protected:
  Message     msg;
  KeyStore    kstore;

  const DeviceInfo& info;
  bool         cfgChanged;  // :1; take ~60byte more flash
  uint8_t      numChannels; // :7;

public:
  Device (const DeviceInfo& i,uint16_t addr,List0Type& l,uint8_t ch) : hal(0), list0(l), msgcount(0), lastmsg(0), kstore(addr), info(i), cfgChanged(false), numChannels(ch) {
#ifdef USE_HW_SERIAL
    device_id[0]=0x00;
#endif
  }
  virtual ~Device () {}

  LedType& led ()  { return hal->led; }
  BatteryType& battery ()  { return hal->battery; }
  const BatteryType& battery () const { return hal->battery; }
  RadioType& radio () { return hal->radio; }
  BuzzerType& buzzer () { return hal->buzzer; }
  KeyStore& keystore () { return this->kstore; }
  Activity& activity () { return hal->activity; }

  Message& message () { return msg; }

  bool hasConfigChanged () {
    bool value = cfgChanged;
    cfgChanged = false;
    return value;
  }

  void hasConfigChanged(bool c) {
    cfgChanged = c;
  }

  void channels (uint8_t num) {
    numChannels = num;
  }

  uint8_t channels () const {
    return numChannels;
  }

  bool hasChannel (uint8_t number) const {
    return number != 0 && number <= channels();
  }


  bool isRepeat(const Message& m) {
    if( m.isRepeated() && lastdev == m.from() && lastmsg == m.count() ) {
      return true;
    }
    // store last message data
    lastdev = m.from();
    lastmsg = m.count();
    return false;
  }

  void setHal (HalType& h) {
    hal = &h;
  }

  HalType& getHal () {
    return *hal;
  }

  StorageConfig getConfigArea () {
    return StorageConfig(kstore.address()-STORAGE_CFG_START);
  }

  uint8_t getConfigByte (uint8_t offset) {
    uint8_t data=0;
#ifdef USE_OTA_BOOTLOADER
    if( offset < 16 ) {
      HalType::pgm_read(&data,OTA_CONFIG_START+offset,1);
    }
#elif defined(DEVICE_CONFIG)
    uint8_t tmp[] = {DEVICE_CONFIG};
    if( offset < sizeof(tmp) ) {
      data = tmp[offset];
    }
#endif
    return data;
  }

  void getDeviceID (HMID& id) {
#ifdef USE_OTA_BOOTLOADER
    HalType::pgm_read((uint8_t*)&id,OTA_HMID_START,sizeof(id));
#elif defined(USE_HW_SERIAL)
    if (device_id[0] == 0x00) {
  #ifdef ARDUINO_ARCH_STM32F1
      uint32_t crc = AskSinBase::crc24((uint8_t*)0x1FFFF7E8,12);
      device_id[0] = (uint8_t)(crc & 0x000000ff);
      device_id[1] = (uint8_t)(crc >> 8 & 0x000000ff);
      device_id[2] = (uint8_t)(crc >> 16 & 0x000000ff);
  #elif defined (ARDUINO_ARCH_STM32) && (defined STM32L1xx)
      uint32_t crc = AskSinBase::crc24((uint8_t*)0x1FF80050, 12);
      device_id[0] = (uint8_t)(crc & 0x000000ff);
      device_id[1] = (uint8_t)(crc >> 8 & 0x000000ff);
      device_id[2] = (uint8_t)(crc >> 16 & 0x000000ff);
  #else
      device_id[0] = boot_signature_byte_get(21);
      device_id[1] = boot_signature_byte_get(22);
      device_id[2] = boot_signature_byte_get(23);
  #endif
    }
    id = HMID(device_id);
#else
    uint8_t ids[3];
    memcpy_P(ids,info.DeviceID,3);
    id = HMID(ids);
#endif
  }

  void getDeviceSerial (uint8_t* serial) {
#ifdef USE_OTA_BOOTLOADER
    HalType::pgm_read((uint8_t*)serial,OTA_SERIAL_START,10);
#elif defined (USE_HW_SERIAL)
  #if defined (ARDUINO_ARCH_STM32F1) || (defined (ARDUINO_ARCH_STM32) && (defined STM32L1xx))
    memcpy_P(serial,info.Serial,4);
    uint8_t* s = serial+4;
    for( int i=0; i<3; ++i ) {
      uint8_t d = device_id[i];
      *s++ = AskSinBase::toChar(d >> 4);
      *s++ = AskSinBase::toChar(d & 0x0f);
    }
  #else
    for (uint8_t i = 0; i < 3; i++) {
      serial[i] = (boot_signature_byte_get(i + 14) % 26) + 65; // Char A-Z
    }
    for (uint8_t i = 3; i < 10; i++) {
      serial[i] = (boot_signature_byte_get(i + 14) % 10) + 48; // Char 0-9
    }
  #endif
#else
    memcpy_P(serial,info.Serial,10);
#endif
  }

  bool isDeviceSerial (const uint8_t* serial) {
    uint8_t tmp[10];
    getDeviceSerial(tmp);
    return memcmp(serial,tmp,10) == 0;
  }

  bool isDeviceID(const HMID& id) {
    HMID me;
    getDeviceID(me);
    return id == me;
  }

  void getDeviceModel (uint8_t* model) {
#ifdef USE_OTA_BOOTLOADER
    HalType::pgm_read(model,OTA_MODEL_START,2);
#else
    memcpy_P(model,info.DeviceModel,sizeof(info.DeviceModel));
#endif
  }

  void getDeviceInfo (uint8_t* di) {
    // first byte is number of channels
#ifdef DEVICE_CHANNEL_COUNT
    *di = DEVICE_CHANNEL_COUNT;
#else
    *di = this->channels();
#endif
    memcpy_P(di+1,info.DeviceInfo,sizeof(info.DeviceInfo));
  }

  HMID getMasterID () {
    return list0.masterid();
  }

  const List0Type& getList0 () {
    return list0;
  }

  bool pollRadio () {
    uint8_t num = radio().read(msg);
    // minimal msg is 10 byte
    // ignore own messages from radio
    if( num >= 10 && isDeviceID(msg.from()) == false ) {
      return process(msg);
    }
    return false;
  }

  uint8_t nextcount () {
    return ++msgcount;
  }

  virtual void configChanged () {}

  virtual bool process(__attribute__((unused)) Message& msg) { return false; }

  bool isBroadcastMsg(Message msg) {
    return (msg.to() == HMID::broadcast && msg.isPairSerial()) || ( msg.isBroadcast() && (msg.isRemoteEvent() || msg.isSensorEvent()) );
  }

  bool send(Message& msg,const HMID& to) {
    msg.to(to);
    getDeviceID(msg.from());
    msg.setRpten(); // has to be set always
    return send(msg);
  }

  bool send(Message& msg) {
    hal->prepareSend(msg);
    bool result = false;
    uint8_t maxsend = list0.transmitDevTryMax();
    bool ledmode = list0.ledMode();
    if( ledmode == 1 ) {
      led().set(LedStates::send);
    }
    while( result == false && maxsend > 0 ) {
      result = radio().write(msg,msg.burstRequired());
      DPRINT(F("<- "));
      msg.dump();
      maxsend--;
      if( result == true && msg.ackRequired() == true && msg.to().valid() == true ) {
        Message response;
        if( (result=waitResponse(msg,response,60)) ) { // 600ms
  #ifdef USE_AES
          if( response.isChallengeAes() == true ) {
            AesChallengeMsg& cm = response.aesChallenge();
            result = processChallenge(msg,cm.challenge(),cm.keyindex());
          }
          else
  #endif
          {
            result = response.isAck();
            // if we got a Nack - we stop trying to send again
            if( response.isNack() ) {
              maxsend = 0;
            }
            // we request wakeup
            // we got the flag to stay awake
            if( msg.isWakeMeUp() || response.isKeepAwake() ) {
              activity().stayAwake(millis2ticks(500));
            }
          }
        }
        DPRINT(F("waitAck: ")); DHEX((uint8_t)result); DPRINTLN(F(""));
      }
    }
    if( ledmode == 1 ) {
      led().set( result == true ? LedStates::ack : LedStates::nack);
    }
    return result;
  }


  void sendAck (Message& msg,uint8_t flag=0x00) {
    msg.ack().init(flag);
    kstore.addAuth(msg);
    send(msg,msg.from());
  }

  void sendAck2 (Message& msg,uint8_t flag=0x00) {
    msg.ack2().init(flag);
    kstore.addAuth(msg);
    send(msg,msg.from());
  }

  void sendNack (Message& msg) {
    msg.nack().init();
    send(msg,msg.from());
  }

  template <class ChannelType>
  void sendAck (Message& msg,ChannelType& ch) {
    msg.ackStatus().init(ch,radio().rssi());
    ch.patchStatus(msg);
    kstore.addAuth(msg);
    send(msg,msg.from());
    ch.changed(false);
  }

  void sendDeviceInfo () {
    sendDeviceInfo(getMasterID(),nextcount());
  }

  void sendDeviceInfo (const HMID& to,uint8_t count) {
    DeviceInfoMsg& pm = msg.deviceInfo();
    pm.init(to,count);
    pm.fill(pgm_read_byte(&info.Firmware),pgm_read_byte(&info.DeviceType));
    getDeviceModel(pm.model());
    getDeviceSerial(pm.serial());
    getDeviceInfo(pm.info());
    send(msg,to);
  }

  void sendSerialInfo (const HMID& to,uint8_t count) {
    SerialInfoMsg& pm = msg.serialInfo();
    pm.init(to,count);
    getDeviceSerial(pm.serial());
    send(msg,to);
  }

  template <class ChannelType>
  void sendInfoActuatorStatus (const HMID& to,uint8_t count,ChannelType& ch,bool ack=true) {
    InfoActuatorStatusMsg& pm = msg.infoActuatorStatus();
    pm.init(count,ch,radio().rssi());
    if( ack == false ) {
      pm.clearAck();
    }
    ch.patchStatus(msg);
    send(msg,to);
    ch.changed(false);
  }

  void sendInfoParamResponsePairs(HMID to,uint8_t count,const GenericList& list) {
    InfoParamResponsePairsMsg& pm = msg.infoParamResponsePairs();
    // setup message for maximal size
    pm.init(count);
    uint8_t  current=0;
    uint8_t* buf=pm.data();
    for( int i=0; i<list.getSize(); ++i ) {
      *buf++ = list.getRegister(i);
      *buf++ = list.getByte(i);
      current++;
      if( current == 8 ) {
        // reset to zero
        current=0;
        buf=pm.data();
        if( send(msg,to) == false ) {
          // exit loop in case of error
          break;
        }
      }
    }
    *buf++ = 0;
    *buf++ = 0;
    current++;
    pm.entries(current);
    pm.clearAck();
    send(msg,to);
  }

  template <class ChannelType>
  void sendInfoPeerList (HMID to,uint8_t count,const ChannelType& channel) {
    InfoPeerListMsg& pm = msg.infoPeerList();
    // setup message for maximal size
    pm.init(count);
    uint8_t  current=0;
    uint8_t* buf=pm.data();
    for( uint8_t i=0; i<channel.peers(); ++i ) {
      Peer p = channel.peerat(i);
      if( p.valid() == true ) {
        memcpy(buf,&p,sizeof(Peer));
        buf+=sizeof(Peer);
        current++;
        if( current == 4 ) {
          // reset to zero
          current=0;
          buf=pm.data();
          if( send(msg,to) == false ) {
            // exit loop in case of error
            break;
          }
        }
      }
    }
    memset(buf,0,sizeof(Peer));
    current++;
    pm.entries(current);
    pm.clearAck();
    send(msg,to);
  }
  
  void sendMasterEvent (Message& msg) {
    send(msg,getMasterID());
    hal->sendPeer();
  }

  template <class ChannelType>
  void sendPeerEvent (Message& msg,const ChannelType& ch) {
    bool sendtopeer=false;
    for( int i=0; i<ch.peers(); ++i ){
      Peer p = ch.peerat(i);
      if( p.valid() == true ) {
        // skip if this is not the first peer of that device
        if( ch.peerfor(p) < i ) {
          continue;
        }
        if( isDeviceID(p) == true ) {
          // we send to ourself - no ack needed
          getDeviceID(msg.from());
          msg.to(msg.from());
          if( msg.ackRequired() == true ) {
            msg.clearAck();
            this->process(msg);
            msg.setAck();
          }
          else {
            this->process(msg);
          }
        }
        else {
          // check if burst needed for peer
          typename ChannelType::List4 l4 = ch.getList4(p);
          msg.burstRequired( l4.burst() );
          send(msg,p);
          sendtopeer = true;
        }
      }
    }
    // if we have no peer - send to master/broadcast
    if( sendtopeer == false ) {
      send(msg,getMasterID());
    }
    // signal that we have send to peer
    hal->sendPeer();
  }

  template <class ChannelType>
  void broadcastPeerEvent (Message& msg,const ChannelType& ch) {
    getDeviceID(msg.from());
    msg.clearAck();
    // check if we are peered to ourself
    if( ch.peerfor(msg.from()) < ch.peers() ) {
      msg.to(msg.from());
      // simply process
      this->process(msg);
    }
    HMID todev;
    bool burst=false;
    // go over all peers, get first external device
    // check if one of the peers needs a burst to wakeup
    for( uint8_t i=0; i<ch.peers(); ++i ) {
      Peer p = ch.peerat(i);
      if( p.valid() == true ) {
        if( msg.from() != p ) {
          if( todev.valid() == false ) {
            todev = p;
          }
          typename ChannelType::List4 l4 = ch.getList4(p);
          burst |= l4.burst();
        }
      }
    }
    // if we have no external device - send to master/broadcast
    if( todev.valid() == false ) {
      todev = getMasterID();
    }
    // DPRINT("BCAST to: ");todev.dump(); DPRINTLN("\n");
    msg.burstRequired(burst);
    msg.setBroadcast();
    send(msg,todev);
    // signal that we have send to peer
    hal->sendPeer();
  }

  template <class ChannelType>
  void broadcastEvent (Message& msg,const ChannelType& ch) {
    broadcastEvent(msg);
  }

  void broadcastEvent (Message& msg) {
    msg.clearAck();
    msg.burstRequired(false);
    msg.setBroadcast();
    send(msg,HMID::broadcast);
    hal->sendPeer();
  }

  void writeList (const GenericList& list,const uint8_t* data,uint8_t length) {
    for( uint8_t i=0; i<length; i+=2, data+=2 ) {
      list.writeRegister(*data,*(data+1));
    }
  }
/*
  bool waitForAck(Message& msg,uint8_t timeout) {
    do {
      if( radio().readAck(msg) == true ) {
        return true;
      }
      _delay_ms(10); // wait 10ms
      timeout--;
    }
    while( timeout > 0 );
    return false;
  }
*/
  bool waitResponse(const Message& msg,Message& response,uint8_t timeout) {
    do {
      uint8_t num = radio().read(response);
      if( num > 0 ) {
        DPRINT(F("-> ")); response.dump();
        if( msg.count() == response.count() &&
            msg.to() == response.from() ) {
          return true;
        }
      }
      _delay_ms(10); // wait 10ms
      timeout--;
    }
    while( timeout > 0 );
    return false;
  }

#ifdef USE_AES
  void sendAckAes (Message& msg,const uint8_t* data) {
    msg.ackAes().init(data);
    send(msg,msg.from());
  }

  bool requestSignature(const Message& msg) {
    // no signature for internal message processing needed
    if( isDeviceID(msg.from()) == true ) {
      return true;
    }
    // signing only possible if sender requests ACK
    if( msg.ackRequired() == true ) {
      AesChallengeMsg signmsg;
      aes128_ctx_t ctx;
      signmsg.init(msg,kstore.getIndex());
      kstore.challengeKey(signmsg.challenge(),kstore.getIndex(),ctx);
      // TODO re-send message handling
      DPRINT(F("<- ")); signmsg.dump();
      radio().write(signmsg,signmsg.burstRequired());
      // read answer
      if( waitForAesResponse(msg.from(),signmsg,60) == true ) {
        AesResponseMsg& response = signmsg.aesResponse();
        uint8_t      initvector[16];
        // DPRINT("AES ");DHEX(response.data(),16);
        // fill initial vector with message to sign
        kstore.fillInitVector(msg,initvector);
        // DPRINT("IV ");DHEX(iv,16);
        // decrypt response
        uint8_t* data = response.data();
        aes128_dec(data,&ctx);
        // xor encrypted data with initial vector
        kstore.applyVector(data,initvector);
        // store data for sending ack
        kstore.storeAuth(response.count(),data);
        // decrypt response
        aes128_dec(data,&ctx);
        // DPRINT("r "); DHEX(response.data()+6,10);
        // DPRINT("s "); DHEX(msg.buffer(),10);
        // compare decrypted message with original message
        if( memcmp(data+6,msg.buffer(),10) == 0 ) {
          DPRINTLN(F("Signature OK"));
          return true;
        }
        else {
          DPRINTLN(F("Signature FAILED"));
        }
      }
      else {
        DPRINTLN(F("waitForAesResponse failed"));
      }
    }
    return false;
  }

  bool processChallenge(const Message& msg,const uint8_t* challenge,uint8_t keyidx) {
    aes128_ctx_t ctx;
    if( kstore.challengeKey(challenge,keyidx,ctx) == true ) {
      DPRINT(F("Process Challenge - Key: "));DHEXLN(keyidx);
      AesResponseMsg answer;
      answer.init(msg);
      uint8_t* data = answer.data();
      {
        uint8_t      initvector[16];
        // fill initial vector with message to sign
        kstore.fillInitVector(msg,initvector);
        for( uint8_t i=0; i<6; ++i ) {
          data[i] = (uint8_t)rand();
        }
        memcpy(data+6,msg.buffer(),10); // TODO - check message to short possible
        aes128_enc(data,&ctx);
        kstore.applyVector(data,initvector);
        aes128_enc(data,&ctx);
      }
      return send(answer,msg.to());
    }
    return false;
  }

  bool waitForAesResponse(const HMID& from,Message& answer,uint8_t timeout) {
    do {
      uint8_t num = radio().read(answer);
      if( num > 0 ) {
        DPRINT(F("-> ")); answer.dump();
        if( answer.isResponseAes() && from == answer.from() ) {
          return true;
        }
      }
      _delay_ms(10); // wait 10ms
      timeout--;
    }
    while( timeout > 0 );
    return false;
  }

#endif
};

}

#endif
