#ifndef __PACKETS_H__
#define __PACKETS_H__

#define PACKET_START (uint8_t)0x80;

typedef enum {
	ePacketTypeIdent = 0xAA,
	ePacketTypeTemperature = 0x20,
	ePacketTypeTemperatureHumidity = 0x21,
	ePacketTypeTrigger = 0x22,
	ePacketTypeTemperatureCO2 = 0x27,
	ePacketTypeKeyboard = 0x28,
} ePacketType;

typedef struct __attribute__((packed))
{
	/* Identity information */
	uint8_t mStart;
	uint8_t mVersion;
	uint8_t mPacketType;
	uint16_t mNodeId;
	uint8_t mBatteryVolts;
	uint32_t mCounter;
} sPacketHeader  ;

typedef struct
{
	uint16_t temperature;
	uint16_t humidity;
} sSensorData;

typedef struct __attribute__((packed))
{
	uint8_t triggerstate;
	
} sTriggerData;

typedef struct __attribute__((packed))
{
	/* Header information */
	sPacketHeader mHeader;
	/* Data */
	sSensorData mSensorData;
	/* Checksum */
	uint8_t mChecksum;
} sDataPacket;

typedef struct __attribute__((packed))
{
	/* Header information */
	sPacketHeader mHeader;
	/* Data */
	sTriggerData mTriggerData;
	/* Checksum */
	uint8_t mChecksum;
} sTriggerPacket;

#endif