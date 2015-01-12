
//#include <includes.h>
#ifndef __SST25V_H
#define __SST25V_H
#include <include.h>
#include <rtthread.h>

extern struct rt_semaphore sem_dataflash;

void sst25_init(void);
void sst25_read(uint32_t addr , uint8_t *p, uint16_t len) ;
void sst25_erase_4k(uint32_t addr);
void sst25_erase_64k( uint32_t addr );
void sst25_write_back(uint32_t addr, uint8_t *p, uint16_t len);
void sst25_write_through(uint32_t addr, uint8_t *p, uint16_t len);
void sst25_write_auto( uint32_t addr, uint8_t *p, uint16_t len , uint32_t addr_start, uint32_t addr_end);
void sst25_read_auto( uint32_t addr, uint8_t *p, uint16_t len , uint32_t addr_start, uint32_t addr_end);


#endif

