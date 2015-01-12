#ifndef _GB_INJECT_
#define _GB_INJECT_

#define FileSize_GBinput   258048



typedef  struct _GB_INJECT
{
    u8    write_flag; //  开始写flag =1 ，擦除flag=2 ，写完了就是0
    u8    run_step; // 执行状态  0 开始
    u16    Pkg_count; // 计数器
    u32   File_size;  //  write file size
    u32   File_left;
    u8     Trans_gb; // 发送固有音频标志位
} GB_INJECT;



extern GB_INJECT  gb_data_inject;  //  导入记录仪数据
extern void gb_inject_run(void);





#endif
