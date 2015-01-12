#ifndef _H_JT808_GPS_PACK_
#define  _H_JT808_GPS_PACK_



void gps_pack_init(void);
void gps_pack_proc( uint8_t *pinfo, uint16_t len );
uint8_t jt808_packt_get( void );

#endif
