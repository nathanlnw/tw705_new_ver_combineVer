#ifndef _GB_INJECT_
#define _GB_INJECT_

#define FileSize_GBinput   258048



typedef  struct _GB_INJECT
{
    u8    write_flag; //  ��ʼдflag =1 ������flag=2 ��д���˾���0
    u8    run_step; // ִ��״̬  0 ��ʼ
    u16    Pkg_count; // ������
    u32   File_size;  //  write file size
    u32   File_left;
    u8     Trans_gb; // ���͹�����Ƶ��־λ
} GB_INJECT;



extern GB_INJECT  gb_data_inject;  //  �����¼������
extern void gb_inject_run(void);





#endif
