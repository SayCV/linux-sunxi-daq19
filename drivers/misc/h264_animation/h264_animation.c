#include <linux/mm.h>
#include <video/sunxi_display2.h>
#include "../../video/sunxi/disp2/disp/de/include.h"
#include "asm/cacheflush.h"
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()??��|kthread_run()
#include <linux/err.h> //IS_ERR()??��|PTR_ERR()
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/ion_sunxi.h>
#include <linux/dma-mapping.h>
#include <linux/ion.h>
#include <linux/version.h>

static __u32 screen_id = 0;
static struct disp_layer_config *g_layer_para;
static void __iomem *decode_base;
//extern long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

s32 uboot_disp_init(u32 nWidth, u32 nHeight)
{
    struct disp_layer_config *layer_para;
    uint screen_width, screen_height;
    uint arg[4];
    //uint32_t full = 0;
    //int nodeoffset;

    layer_para = (struct disp_layer_config *)kmalloc(sizeof(struct disp_layer_config), GFP_KERNEL);
    if(!layer_para)
    {
        printk(KERN_EMERG "\nsunshijie: init display error: unable to malloc memory for layer\n");
        return -1;
    }
#if 0
    nodeoffset = fdt_path_offset(working_fdt,"boot_disp");
    if (nodeoffset >= 0)
    {
        if(fdt_getprop_u32(working_fdt,nodeoffset,"output_full",&full) < 0)
        {
            printf("fetch script data boot_disp.output_full fail\n");
        }
    }
#endif
    arg[0] = screen_id;//??????
    //screen_width = disp_ioctl(NULL, DISP_GET_SCN_WIDTH, (void*)arg);
    //screen_height = disp_ioctl(NULL, DISP_GET_SCN_HEIGHT, (void*)arg);
    screen_width = 1024;
    screen_height = 600;
    memset((void *)layer_para, 0, sizeof(struct disp_layer_config));
    layer_para->channel = 0;
    layer_para->layer_id = 0;
    layer_para->info.fb.format      = DISP_FORMAT_YUV420_P;

    layer_para->info.fb.size[0].width   = nWidth;
    layer_para->info.fb.size[0].height  = nHeight;
    layer_para->info.fb.size[1].width   = nWidth/2;
    layer_para->info.fb.size[1].height  = nHeight/2;
    layer_para->info.fb.size[2].width   = nWidth/2;
    layer_para->info.fb.size[2].height  = nHeight/2;
    layer_para->info.fb.crop.x  = 0;
    layer_para->info.fb.crop.y  = 0;
    layer_para->info.fb.crop.width  = ((unsigned long long)nWidth) << 32;
    layer_para->info.fb.crop.height = ((unsigned long long)nHeight) << 32;
    layer_para->info.fb.align[0] = 16;
    layer_para->info.fb.align[1] = 16;
    layer_para->info.fb.align[2] = 16;

    layer_para->info.fb.flags = DISP_BF_NORMAL;
    layer_para->info.fb.scan = DISP_SCAN_PROGRESSIVE;
    layer_para->info.mode     = LAYER_MODE_BUFFER;
    layer_para->info.alpha_mode    = 1;
    layer_para->info.alpha_value   = 0xff;

    layer_para->info.screen_win.x   = 0;
    layer_para->info.screen_win.y   = 0;
    layer_para->info.screen_win.width   = screen_width;
    layer_para->info.screen_win.height  = screen_height;

    layer_para->info.b_trd_out      = 0;
    layer_para->info.out_trd_mode   = 0;
    g_layer_para = layer_para;
    return 0;
}
extern struct disp_layer_config * get_config_car_reverse(int chan);
extern struct disp_manager *disp_get_layer_manager(u32 disp);
int uboot_display_layer_open(void)
{
    uint arg[4];
    struct disp_layer_config *config = g_layer_para;
    
    arg[0] = screen_id;//????
    arg[1] = (unsigned long)config;
    arg[2] = 1;
    arg[3] = 0;
    struct disp_manager *mgr = disp_get_layer_manager(0);
    mgr->force_set_layer_config(mgr, config, 1);
    //disp_ioctl(NULL,DISP_LAYER_GET_CONFIG,(void*)arg);
    config->enable = 1;
    //disp_ioctl(NULL,DISP_LAYER_SET_CONFIG,(void*)arg);
    mgr->force_set_layer_config(mgr, config, 1);
    return 0;
}

int uboot_display_layer_close(void)
{
    uint arg[4];
    struct disp_layer_config *config = g_layer_para;

    arg[0] = screen_id;//????
    arg[1] = (unsigned long)config;
    arg[2] = 1;
    arg[3] = 0;

    struct disp_manager *mgr = disp_get_layer_manager(0);
    mgr->force_set_layer_config(mgr, config, 1);
    //disp_ioctl(NULL,DISP_LAYER_GET_CONFIG,(void*)arg);
    config->enable = 0;
    mgr->force_set_layer_config(mgr, config, 1);
    //disp_ioctl(NULL,DISP_LAYER_SET_CONFIG,(void*)arg);
    return 0;
}


int uboot_disp_frame_update(u32 pFrameBuffer, u32 nWidth, u32 nHeight)
{
    struct disp_layer_config *layer_para;
    layer_para = g_layer_para;
    if(pFrameBuffer != 0)
    {
        layer_para->info.fb.addr[0] = (uint)pFrameBuffer;//sunsj
        layer_para->info.fb.addr[1] = (uint)pFrameBuffer+nWidth*nHeight*5/4;
        layer_para->info.fb.addr[2] = (uint)pFrameBuffer+nWidth*nHeight;
    }
    else
    {
        return -1;
    }
    return 0;
}

int uboot_display_layer_para_set(void)
{
    uint arg[4];

    arg[0] = 0;//????
    arg[1] = (uint)g_layer_para;
    arg[2] = 1;
    arg[3] = 0;
    struct disp_manager *mgr = disp_get_layer_manager(0);
    mgr->force_set_layer_config(mgr, g_layer_para, 1);
    //disp_ioctl(NULL,DISP_LAYER_SET_CONFIG,(void*)arg);
    return 0;
}

void uboot_ve_init(void)//sunsj
{
    printk(KERN_EMERG "\nsunshijie: uboot_ve_init in\n");
    /*start cdcVeInit*/
    u32  val;
    volatile u32* pReg_addr ;
    void __iomem *ve_base;
    int offset;
    ve_base = ioremap(0x01c20000, 0x2c8);
    printk(KERN_EMERG "\nsunshijie: uboot_ve_init ve_base = 0x%p\n", ve_base);
    
    val = 0;
    //pReg_addr = (volatile u32*)0x01c20018;
    offset = 0x18;
    pReg_addr = ve_base + offset;
    val = readl(pReg_addr);
    val |= (1<<31); /*set VE clock dividor*/
    writel(val, pReg_addr);

    val = 0;
    //pReg_addr = (volatile u32*)0x01c2013c;
    offset = 0x13c;
    pReg_addr = ve_base + offset;
    val = readl(pReg_addr);
    val |= (1<<31); /*open ve_clk gating*/
    writel(val, pReg_addr);

    val = 0;
    //pReg_addr = (volatile u32*)0x01c20064;
    offset = 0x64;
    pReg_addr = ve_base + offset;
    val = readl(pReg_addr);
    val |= (1<<0); /*Active AHB bus to MACC*/
    writel(val, pReg_addr);

    val = 0;
    //pReg_addr = (volatile u32*)0x01c202c4;
    offset = 0x2c4;
    pReg_addr = ve_base + offset;
    val = readl(pReg_addr);
    val &= 0xfffffffe;/*reset ve*/
    writel(val, pReg_addr);

    val = 0;
    //pReg_addr = (volatile u32*)0x01c202c4;
    offset = 0x2c4;
    pReg_addr = ve_base + offset;
    val = readl(pReg_addr);
    val |= 0x00000001;/*power on ve*/
    writel(val, pReg_addr);

    val = 0;
    //pReg_addr = (volatile u32*)0x01c20100;
    offset = 0x100;
    pReg_addr = ve_base + offset;
    val = readl(pReg_addr);
    val |= 0x00000001;/*gate on the bus to SDRAM*/
    writel(val, pReg_addr);
    printk(KERN_EMERG "\nsunshijie: uboot_ve_init out\n");
}

int uboot_disp_show(int display_source)//sunsj
{
    uboot_display_layer_para_set();
    uboot_display_layer_open();
    return 0;
}

void uboot_print_register(void)
{
    u32 i = 0;
    volatile u32* reg_base = NULL;
    volatile u32* TopRegister;
    volatile u32* BaseRegister;

    TopRegister  = (volatile u32*)0x01c0e000;
    BaseRegister = (volatile u32*)0x01c0e200;

    reg_base = TopRegister;
    printk(KERN_EMERG "\nsunshijie: **************top register=0x%p\n",reg_base);

    for(i=0;i<16;i++)
    {
        printk(KERN_EMERG "%02x, 0x%08x 0x%08x 0x%08x 0x%08x \n",
               i, *(reg_base+4*i),*(reg_base+4*i+1),*(reg_base+4*i+2),*(reg_base+4*i+3));
    }
    printk(KERN_EMERG "\n");

    reg_base = BaseRegister;
    printk(KERN_EMERG "\nsunshijie: **************base register=0x%p\n",reg_base);

    for(i=0;i<16;i++)
    {
        printk(KERN_EMERG "%02x, 0x%08x 0x%08x 0x%08x 0x%08x \n",
               i, *(reg_base+4*i),*(reg_base+4*i+1),*(reg_base+4*i+2),*(reg_base+4*i+3));
    }
    printk(KERN_EMERG " \n ");
    printk(KERN_EMERG " \n ");
    printk(KERN_EMERG " \n ");
}

int uboot_decode_H264(u32* Reg, u32 nWidth, u32 nHeight)//sunsj
{
    u32 i;
    u32 val;
    volatile u32* pInitRegBase;
    volatile u32* BaseRegister;
    int offset;
    //BaseRegister = (volatile u32*)0x01c0e0ec;
    offset = 0xe0ec;
    BaseRegister = decode_base + offset;
    printk(KERN_EMERG "\nsunshijie: uboot_decode_H264 BaseRegister = %x\n", BaseRegister);
    writel(0x00000030, BaseRegister); //YV12;

    val = (nWidth*nHeight)>>2;
    //BaseRegister = (volatile u32*)0x01c0e0c4;
    offset = 0xe0c4;
    BaseRegister = decode_base + offset;
    printk(KERN_EMERG "\nsunshijie: uboot_decode_H264 BaseRegister = %x\n", BaseRegister);
    writel(val, BaseRegister); //nRefCSize;

    val = (nWidth>>1)<<16; //CLineStride;
    val |= nWidth;  //YLineStride;
    //BaseRegister = (volatile u32*)0x01c0e0c8;
    offset = 0xe0c8;
    BaseRegister = decode_base + offset;
    printk(KERN_EMERG "\nsunshijie: uboot_decode_H264 BaseRegister = %x\n", BaseRegister);
    writel(val, BaseRegister);

    for(i=0; ; i++)
    {
        if(Reg[2*i]==0x0)
        {
            printk(KERN_EMERG "\nsunshijie: uboot_decode_H264 config the end,trigger ve\n");
            break;
        }
        pInitRegBase = (volatile u32*)Reg[2*i];//sunsj ioremap(pInitRegBase) and virt to phy(Reg[2*i+1])???
        offset = Reg[2*i] & 0xffff;
        printk(KERN_EMERG "\nsunshijie: pInitRegBase(phy) = %x, reg[%d] = %x, offset = %x\n", pInitRegBase, 2*i+1, Reg[2*i+1], offset);
        pInitRegBase = decode_base + offset;
        printk(KERN_EMERG "\nsunshijie: pInitRegBase(virt) = %x\n", pInitRegBase);
        writel(Reg[2*i+1], pInitRegBase);
        //printf("the_conf_reg,%d,%p=%x\n",i,pInitRegBase,Reg[2*i+1]);
    }

    //BaseRegister = (volatile u32*)0x01c0e228;
    offset = 0xe228;
    BaseRegister = decode_base + offset;
    while(1)
    {
        val = readl(BaseRegister);
        if((val&0x01) > 0)    //decode finish;
        {
            printk(KERN_EMERG "\nsunshijie: uboot_decode_H264 decode finish\n");
            writel(val, BaseRegister); //clear register;
            break;
        }
        //__udelay(5);
    }
    //MjpegPrintRegister();
    return 0;
}

void replaceXaddr(u32* regvalue,u32* pXaddr)//sunsj
{
    int j;
    u32 nWidth;
    u32 nHeight;
    u32 pMvBuf;
    u32 pMvOffset;
    u32 pNeiBuf;
    u32 pRegAddr;
    u32 pFrameBuf;
    u32 pStream;
    u32 pStreamEnd;
    u8  nCount_e4 = 0;

    pStream    = pXaddr[0];
    pStreamEnd = pXaddr[1];
    pNeiBuf    = pXaddr[2];
    pFrameBuf  = pXaddr[3];
    pMvBuf     = pXaddr[4];
    pMvOffset  = pXaddr[5];
    nWidth     = pXaddr[6];
    nHeight    = pXaddr[7];

    for(j=0; ;j++)
    {
        pRegAddr = regvalue[j*2];
        if(pRegAddr == 0x00)
        {
            break;
        }
        //printf("before replace:0x%8x,0x%8x\n",regvalue[j*2],regvalue[j*2+1]);
        if(pRegAddr==0x01c0e230)//bitstream addr
        {
            regvalue[j*2+1] = pStream;
        }
        else if(pRegAddr==0x01c0e23c)//bitstream end
        {
            regvalue[j*2+1] = pStreamEnd;
        }
        else if(pRegAddr==0x01c0e250)//pMbFieldIntraBuf
        {
            regvalue[j*2+1] = pNeiBuf;
        }
        else if(pRegAddr==0x01c0e254)//pMbNeighborInfoBuf
        {
            regvalue[j*2+1] = pNeiBuf+0x80000;
        }
        else if(pRegAddr==0x01c0e2e4)
        {
            if(regvalue[j*2+1]==0)
            {
                continue;//don't know how to config
            }
            else
            {
                if(nCount_e4 == 0)
                {
                    regvalue[j*2+1] = pFrameBuf;//Y_buffer
                    nCount_e4++;
                }
                else if(nCount_e4 == 1)
                {
                    regvalue[j*2+1] = pFrameBuf+nWidth*nHeight;//C_buffer
                    nCount_e4++;
                }
                else if(nCount_e4 == 2)
                {
                    regvalue[j*2+1] = pMvBuf;//pMv buffer
                    nCount_e4++;
                }
                else if(nCount_e4 == 3)
                {
                    regvalue[j*2+1] = pMvBuf+pMvOffset;//pMv offset
                    nCount_e4++;
                }
            }
        }
    }
}
extern struct file *fiilp_open(const char *filename, int flags, umode_t mode);
extern int filp_close(struct file *filp, fl_owner_t id);
extern ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
extern ssize_t vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos);
static int h264_read(char * file_path, void * buffer, int size)
{
    printk(KERN_EMERG "\nsunshijie: h264_read in\n");
    struct file * file_p = filp_open(file_path, O_RDWR, 0644);
    mm_segment_t fs_old;
    loff_t pos = 0;
    int buf_len = 0;
    if(IS_ERR(file_p))
    {
        printk(KERN_EMERG "\nsunshijie: h264_read file_p err!!!\n");
        return -1;
    }
    fs_old = get_fs();
    set_fs(KERNEL_DS);

    buf_len = vfs_read(file_p, (char *)buffer, size, &pos);
    printk(KERN_EMERG "\nsunshijie: h264_read buf_len = %d\n", buf_len);
    filp_close(file_p, NULL);
    
    set_fs(fs_old);
    printk(KERN_EMERG "\nsunshijie: h264_read out\n");
    return 0;
}
static int h264_write(char * file_path, void * buffer, int size)
{
    printk(KERN_EMERG "\nsunshijie: h264_write in\n");
    struct file * file_p = filp_open(file_path, O_RDWR | O_CREAT, 0);
    mm_segment_t fs_old;
    loff_t pos = 0;
    int buf_len = 0;
    if(IS_ERR(file_p))
    {
        printk(KERN_EMERG "\nsunshijie: h264_write file_p err!!!\n");
        return -1;
    }
    fs_old = get_fs();
    set_fs(KERNEL_DS);

    buf_len = vfs_write(file_p, (char *)buffer, size, &pos);
    printk(KERN_EMERG "\nsunshijie: h264_write buf_len = %d\n", buf_len);
    filp_close(file_p, NULL);
    
    set_fs(fs_old);
    printk(KERN_EMERG "\nsunshijie: h264_write out\n");
    return 0;
}
//int rootfs_mounted = 0;
static struct h264_ion {
    struct ion_handle * handle;
    int size;
    void *vir_address;
    unsigned long phy_address;
};
static struct h264_ion * h264_ion_alloc(int size)
{
    printk(KERN_EMERG "\nsunshijie: disp_frame_alloc\n");
    struct h264_ion * ion_p = NULL;
    int alloc_size = PAGE_ALIGN(size);
    struct ion_client *client = sunxi_ion_client_create("buffer-pool");
    if (!client || !size)
		return NULL;
    ion_p = kzalloc(sizeof(struct h264_ion), GFP_KERNEL);
    if (!ion_p) {
		printk(KERN_EMERG "\nsunshijie: h264_ion_alloc ion_p failed\n");
		return NULL;
	}
    ion_p->handle = ion_alloc(client, alloc_size, PAGE_SIZE,	ION_HEAP_CARVEOUT_MASK|ION_HEAP_TYPE_DMA_MASK, 0);
    if (IS_ERR_OR_NULL(ion_p->handle)) {
		printk(KERN_EMERG "\nsunshijie: h264_ion_alloc failed\n");
		return NULL;
	}
    ion_p->vir_address = ion_map_kernel(client, ion_p->handle);
    if (IS_ERR_OR_NULL(ion_p->vir_address)) {
		printk(KERN_EMERG "\nsunshijie: h264_ion_alloc ion_map_kernel failed\n");
		return NULL;
	}
    if (ion_phys(client, ion_p->handle, (ion_phys_addr_t *)&ion_p->phy_address, &alloc_size)) {
		printk(KERN_EMERG "\nsunshijie: h264_ion_alloc ion_phys failed\n");
		return NULL;
	}
    ion_p->size = alloc_size;
    return ion_p;
}
int sunxi_uboot_display(void)
{
    printk("KERN_EMERG \nsunshijie: sunxi_uboot_display in\n");
    //while(!rootfs_mounted)
    //    msleep(500);
    //printk(KERN_EMERG "\nsunshijie: rootfs_mounted = %d\n", rootfs_mounted);
    s32   ret;
    u32   tmp;
    u32   pMvBuf;
    u32   pNeiBuf;
    u32   pStreamBuf;
    u32   pFrameBuf;
    u32   pFrameBuf_0;
    u32   pFrameBuf_1;
    u32   pPhyStreamEnd;
    u8*   pMvBuf_temp = NULL;
    u8*   pNeiBuf_temp = NULL;
    u8*   pFrameBuf_tmp_0 = NULL;
    u8*   pFrameBuf_tmp_1 = NULL;
    u8*   pStreamBuf_temp = NULL;
    u32   nDecFrame = 1;
    u32   regvalue[600];
    u8    nFrameIdx;
    char *argv[8];
    char filename[32];
    char bmp_head[32];
    u32  pXbufferAddr[8];//0:sbmaddr;1:sbmEndAddr;2:neibor;3:frame;4:MV;5:MV_offset;6:width;7:height;
    u32  nWidth,nMbWidth;
    u32  nHeight,nMbHeight;
    u32  fieldMvColBufSize;
    //unsigned long time;

    nWidth = 1920;
    nHeight = 1088;
    nMbWidth = nWidth/16;
    nMbHeight = nHeight/16;
    fieldMvColBufSize = nMbHeight;
    fieldMvColBufSize = (fieldMvColBufSize+1)/2;
    fieldMvColBufSize = nMbWidth*fieldMvColBufSize*32;
    fieldMvColBufSize *= 2;
    pXbufferAddr[5] = fieldMvColBufSize;
    pXbufferAddr[6] = nWidth;
    pXbufferAddr[7] = nHeight;

    /******************step1:malloc buffer********************/
    void * v_addr;
    //pStreamBuf_temp = (u8*)kmalloc(1024*1024+1023, GFP_KERNEL);
    //v_addr = (void *)kmalloc(1024*1024+1023, GFP_KERNEL);
    //pStreamBuf_temp = (u32)virt_to_phys(v_addr);
    struct h264_ion * pStreamBuf_ion = h264_ion_alloc(1024*1024+1023);
    pStreamBuf_temp = pStreamBuf_ion->phy_address;
    printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display pStreamBuf_ion(phy) = %x, pStreamBuf_ion(virt) = %x\n", pStreamBuf_ion->phy_address, pStreamBuf_ion->vir_address);
    if(pStreamBuf_temp == NULL)
    {
        goto decode_error;
    }
    pStreamBuf = (u32)(((u32)pStreamBuf_temp+1023)&~1023);
    v_addr = (void *)(((u32)(pStreamBuf_ion->vir_address)+1023)&~1023);
    tmp = (u32)(pStreamBuf & 0xffffffff);
    tmp = (tmp>>28) | (tmp&0x0ffffff0);
    pPhyStreamEnd = (u32)((u8*)pStreamBuf+1024*1024-1);
    pXbufferAddr[0] = 7<<28 | tmp;
    pXbufferAddr[1] = pPhyStreamEnd;
    
    //pFrameBuf_tmp_0 = (u8*)kmalloc(nWidth*nHeight*3/2 + 1023, GFP_KERNEL);
    //v_addr = (void *)kmalloc(nWidth*nHeight*3/2 + 1023, GFP_KERNEL);
    //pFrameBuf_tmp_0 = (u32)virt_to_phys(v_addr);
    //printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display v_addr = 0x%p, pFrameBuf_tmp_0 = %x\n", v_addr, pFrameBuf_tmp_0);
    struct h264_ion * pFrameBuf_tmp_0_ion = h264_ion_alloc(nWidth*nHeight*3/2+1023);
    pFrameBuf_tmp_0 = pFrameBuf_tmp_0_ion->phy_address;
    printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display pFrameBuf_tmp_0_ion(phy) = %x, pFrameBuf_tmp_0_ion(virt) = %x\n", pFrameBuf_tmp_0_ion->phy_address, pFrameBuf_tmp_0_ion->vir_address);
    if(pFrameBuf_tmp_0 == NULL)
    {
        goto decode_error;
    }
    pFrameBuf_0 = (u32)(((u32)pFrameBuf_tmp_0+1023)&~1023);

    //pFrameBuf_tmp_1 = (u8*)kmalloc(nWidth*nHeight*3/2 + 1023, GFP_KERNEL);
    //v_addr = (void *)kmalloc(nWidth*nHeight*3/2 + 1023, GFP_KERNEL);
    //pFrameBuf_tmp_1 = (u32)virt_to_phys(v_addr);
    //printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display v_addr = 0x%p, pFrameBuf_tmp_1 = %x\n", v_addr, pFrameBuf_tmp_1);
    struct h264_ion * pFrameBuf_tmp_1_ion = h264_ion_alloc(nWidth*nHeight*3/2+1023);
    pFrameBuf_tmp_1 = pFrameBuf_tmp_1_ion->phy_address;
    printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display pFrameBuf_tmp_1_ion(phy) = %x, pFrameBuf_tmp_1_ion(virt) = %x\n", pFrameBuf_tmp_1_ion->phy_address, pFrameBuf_tmp_1_ion->vir_address);
    if(pFrameBuf_tmp_1 == NULL)
    {
        goto decode_error;
    }
    pFrameBuf_1 = (u32)(((u32)pFrameBuf_tmp_1+1023)&~1023);
    
    //pMvBuf_temp = (u8*)kmalloc(fieldMvColBufSize*2 + 1023, GFP_KERNEL);
    //v_addr = (void *)kmalloc(fieldMvColBufSize*2 + 1023, GFP_KERNEL);
    //pMvBuf_temp = (u32)virt_to_phys(v_addr);
    //printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display v_addr = 0x%p, pMvBuf_temp = %x\n", v_addr, pMvBuf_temp);
    struct h264_ion * pMvBuf_temp_ion = h264_ion_alloc(fieldMvColBufSize*2+1023);
    pMvBuf_temp = pMvBuf_temp_ion->phy_address;
    printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display pMvBuf_temp_ion(phy) = %x, pMvBuf_temp_ion(virt) = %x\n", pMvBuf_temp_ion->phy_address, pMvBuf_temp_ion->vir_address);
    if(pMvBuf_temp == NULL)
    {
        goto decode_error;
    }
    pMvBuf = (u32)(((u32)pMvBuf_temp+1023)&~1023);
    pXbufferAddr[4] = pMvBuf;

    //pNeiBuf_temp = (u8*)kmalloc(1024*1024 + 1023, GFP_KERNEL);
    //v_addr = (void *)kmalloc(1024*1024 + 1023, GFP_KERNEL);
    //pNeiBuf_temp = (u32)virt_to_phys(v_addr);
    //printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display v_addr = 0x%p, pNeiBuf_temp = %x\n", v_addr, pNeiBuf_temp);
    struct h264_ion * pNeiBuf_temp_ion = h264_ion_alloc(1024*1024+1023);
    pNeiBuf_temp = pNeiBuf_temp_ion->phy_address;
    printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display pNeiBuf_temp_ion(phy) = %x, pNeiBuf_temp_ion(virt) = %x\n", pNeiBuf_temp_ion->phy_address, pNeiBuf_temp_ion->vir_address);
    if(pNeiBuf_temp == NULL)
    {
        goto decode_error;
    }
    pNeiBuf = (u32)(((u32)pNeiBuf_temp+1023)&~1023);
    pXbufferAddr[2] = pNeiBuf;

    /******************step2:init********************/
    nFrameIdx = 0;
    uboot_ve_init();//init register about VE;
    ret = uboot_disp_init(nWidth, nHeight);//init disp para;
    if(ret < 0)
    {
        goto decode_error;
    }

    argv[0] = "fatload";//sunsj
    argv[1] = "sunxi_flash";
    argv[2] = "0:0";
    argv[3] = bmp_head;
    argv[4] = filename;
    argv[5] = NULL;
    int i;
    for(i = 0; i < 8; i ++) //i = 3 not init
    {
        printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display pXbufferAddr[%d] = %x\n", i, pXbufferAddr[i]);
    }
    printk("KERN_EMERG \nsunshijie: sunxi_uboot_display init ok\n");
    decode_base = ioremap(0x01c00000, 0xe2e8);
    printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display decode_base = %x\n",decode_base);
    while(1)
    {
        printk("KERN_EMERG \nsunshijie: sunxi_uboot_display update frame\n");
        //time = get_timer(0);
        /******************step3:decode frame********************/
        sprintf(filename, "/bat/aw_bitstream_%d.dat", nDecFrame);
        sprintf(bmp_head, "%lx", (ulong)pStreamBuf);
        //memset((u8*)pStreamBuf, 0, 1024*1024); //read bitstream;
        //if (do_fat_fsload(0, 0, 5, argv))
        //v_addr = phys_to_virt((ulong)pStreamBuf);
        printk(KERN_EMERG "\nsunshijie: v_addr = %x, pStreamBuf = %x\n", v_addr, pStreamBuf);
        memset((u8 *)v_addr, 0, 1024*1024); //read bitstream;
        if (h264_read(filename, v_addr, 1024*1024))
        {
            printk(KERN_EMERG "\nsunshijie: sunxi bmp info error : unable to open logo file %s\n", argv[4]);
            goto decode_error;
        }
        //flush_cache((ulong)pStreamBuf, (ulong)1024*1024);
        flush_cache_all();
		#if 0
        //for test begin
        sprintf(filename, "/tmp/aw_bitstream_%dTEST.dat", nDecFrame);
        h264_write(filename, v_addr, 1024*1024);
        flush_cache_all();
        //for test end
		#endif
        sprintf(filename, "/bat/register_%d.dat", nDecFrame);
        sprintf(bmp_head, "%lx", (ulong)regvalue);
        argv[3] = bmp_head;
        argv[4] = filename;
        argv[5] = NULL;
        memset(regvalue, 0, 2400);
        //if (do_fat_fsload(0, 0, 5, argv))//read register config;
        if (h264_read(filename, (void *)(&regvalue[0]), 2400))//read register config;
        {
            printk(KERN_EMERG "\nsunshijie: sunxi bmp info error : unable to open logo file %s\n", argv[4]);
            goto decode_error;
        }
        flush_cache_all();
		#if 0
        //for test begin
        sprintf(filename, "/tmp/register_%dTEST.dat", nDecFrame);
        h264_write(filename, (void *)(&regvalue[0]), 2400);
        flush_cache_all();
        //for test end
		#endif

        if(nFrameIdx == 0)
        {
            pFrameBuf = pFrameBuf_0;
            nFrameIdx = 1;
        }
        else if(nFrameIdx == 1)
        {
            pFrameBuf = pFrameBuf_1;
            nFrameIdx = 0;
        }
        else
        {
            printk(KERN_EMERG "\nsunshijie: Frame biffer use error,exit decoder!\n");
            goto decode_error;
        }
        pXbufferAddr[3] = pFrameBuf;
        printk(KERN_EMERG "\nsunshijie: sunxi_uboot_display pFrameBuf = %x\n", pFrameBuf);

        replaceXaddr(regvalue,pXbufferAddr);
        uboot_decode_H264(regvalue,nWidth,nHeight);
        //flush_cache((ulong)pFrameBuf, (ulong)nWidth*nHeight*3/2);
        flush_cache_all();
        nDecFrame++;

        /******************step4:display the frame********************/
        uboot_disp_frame_update(pFrameBuf,nWidth,nHeight);
        //time = get_timer(time);
        if(nDecFrame > 1)
        {
            //__udelay(((unsigned long)33 - time)*1000);    //set the frame rate 30,33ms=1000ms/30
            mdelay(10);
        }
        uboot_disp_show(0);
    }
decode_error:
    /******************step5:free malloc buffer********************/
    //__udelay(33000);
    mdelay(33);
    uboot_display_layer_close();
    //sunxi_bmp_display("bat\\bootlogo.bmp");
    if(pStreamBuf_temp!=NULL)
    {
        kfree(pStreamBuf_temp);
        pStreamBuf_temp = NULL;
    }
    if(pFrameBuf_tmp_0!=NULL)
    {
        kfree(pFrameBuf_tmp_0);
        pFrameBuf_tmp_0 = NULL;
    }
    if(pFrameBuf_tmp_1!=NULL)
    {
        kfree(pFrameBuf_tmp_1);
        pFrameBuf_tmp_1 = NULL;
    }
    if(pMvBuf_temp!=NULL)
    {
        kfree(pMvBuf_temp);
        pMvBuf_temp = NULL;
    }
    if(pNeiBuf_temp!=NULL)
    {
        kfree(pNeiBuf_temp);
        pNeiBuf_temp = NULL;
    }
    printk("KERN_EMERG \nsunshijie: sunxi_uboot_display out\n");
    return 1;
}
static int __init EnterUbootDisMode(void)
{
    printk(KERN_EMERG "\nsunshijie: EnterUbootDisMode\n");
    //sunxi_uboot_display();
    kthread_run(sunxi_uboot_display, NULL, "animation");
    return 0;
}

module_init(EnterUbootDisMode);

MODULE_AUTHOR("sunsj");
MODULE_LICENSE("GPL");
