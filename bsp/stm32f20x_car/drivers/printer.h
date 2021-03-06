#ifndef _PRINTER_H_
#define _PRINTER_H_


#define PRINTER_CMD_GRAYLEVEL			0x6d
#define PRINTER_CMD_DOTLINE_N			0x4a
#define PRINTER_CMD_CHRLINE_N			0x64
#define PRINTER_CMD_LINESPACE_N			0x33
#define PRINTER_CMD_LINESPACE_DEFAULT	0x32
#define PRINTER_CMD_MARGIN_LEFT			0x6C
#define PRINTER_CMD_MARGIN_RIGHT		0x51
#define PRINTER_CMD_FACTORY				0x40
#define PRINTER_CMD_PAPER				0xFF

extern uint8_t		print_state;		//0表示没有打印，1表示正在打印


void printer( const char *str );
void printer_driver_init( void );

#endif
