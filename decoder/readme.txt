MPP 版本生成库时，Makefile.param 加入 -ffunction-sections 编译选项；客户在
链接生成应用程序时加入 -Wl,-gc-sections，能有效减小应用程序大小，剔除掉没有使
用到的函数。

需解码 B帧的通道设置不支持 B帧解码 HI_MPI_VDEC_CreateChn不需要为解码通道分配用于输出 pmv 的buffe