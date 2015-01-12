/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */

DSTATUS disk_initialize_USB (BYTE drv);
DSTATUS disk_status_USB (BYTE drv);
DSTATUS disk_initialize_USB (BYTE drv);
DRESULT disk_read_USB (BYTE drv, BYTE *buff, DWORD sector, BYTE count);
DRESULT disk_write_USB (BYTE drv, const BYTE *buff, DWORD sector, BYTE count);
DRESULT disk_ioctl_USB(BYTE drv, BYTE ctrl, void *buff);



/*-----------------------------------------------------------------------*/
/* Inicializes a Drive                                                    */

DSTATUS disk_initialize (BYTE drv)    /* Physical drive nmuber (0..) */
{
    DSTATUS stat = STA_NOINIT;
    int result = STA_NOINIT;

    switch (drv)
    {
    case ATA :
        //result = disk_initialize_SD();
        // translate the reslut code here

        return stat;

    case MMC :
        stat = disk_initialize_SD(drv);
        // translate the reslut code here

        return stat;

    case USB :
        stat = disk_initialize_USB(drv);
        // translate the reslut code here

        return stat;
    }
    return STA_NOINIT;

}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
    BYTE drv		/* Physical drive nmuber (0..) */
)
{
    DSTATUS stat = STA_NOINIT;
    int result = STA_NOINIT;

    switch (drv)
    {
    case ATA :
        //result = ATA_disk_status();
        // translate the reslut code here

        return stat;

    case MMC :
        stat = disk_status_SD(drv);
        // translate the reslut code here

        return stat;

    case USB :
        stat = disk_status_USB(drv);
        // translate the reslut code here

        return stat;
    }
    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
    BYTE drv,		/* Physical drive nmuber (0..) */
    BYTE *buff,		/* Data buffer to store read data */
    DWORD sector,	/* Sector address (LBA) */
    BYTE count		/* Number of sectors to read (1..255) */
)
{
    DRESULT stat = STA_NOINIT;
    int result = STA_NOINIT;

    switch (drv)
    {
    case ATA :
        //result = ATA_disk_read(buff, sector, count);
        // translate the reslut code here

        return stat;

    case MMC :
        stat = disk_read_SD(drv, buff, sector, count);
        // translate the reslut code here

        return stat;

    case USB :
        stat = disk_read_USB(drv, buff, sector, count);
        // translate the reslut code here

        return stat;
    }
    return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
    BYTE drv,			/* Physical drive nmuber (0..) */
    const BYTE *buff,	/* Data to be written */
    DWORD sector,		/* Sector address (LBA) */
    BYTE count			/* Number of sectors to write (1..255) */
)
{
    DRESULT stat = STA_NOINIT;
    int result = STA_NOINIT;

    switch (drv)
    {
    case ATA :
        //result = ATA_disk_write(buff, sector, count);
        // translate the reslut code here

        return stat;

    case MMC :
        stat = disk_write_SD(drv, buff, sector, count);
        // translate the reslut code here

        return stat;

    case USB :
        stat = disk_write_USB(drv, buff, sector, count);
        // translate the reslut code here

        return stat;
    }
    return RES_PARERR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
    BYTE drv,		/* Physical drive nmuber (0..) */
    BYTE ctrl,		/* Control code */
    void *buff		/* Buffer to send/receive control data */
)
{
    DRESULT stat = STA_NOINIT;
    int result = STA_NOINIT;

    switch (drv)
    {
    case ATA :
        // pre-process here

        //result = ATA_disk_ioctl(ctrl, buff);
        // post-process here

        return stat;

    case MMC :
        // pre-process here

        stat = disk_ioctl_SD(drv, ctrl, buff);
        // post-process here

        return stat;

    case USB :
        // pre-process here

        stat = disk_ioctl_USB(drv, ctrl, buff);
        // post-process here

        return stat;
    }
    return RES_PARERR;
}

