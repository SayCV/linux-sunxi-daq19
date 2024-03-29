#include <linux/compat.h>
#include <asm/uaccess.h>
#include "nand_blk.h"
#include "nand_dev.h"

/*****************************************************************************/

#define REMAIN_SPACE 0
#define PART_FREE 0x55
#define PART_DUMMY 0xff
#define PART_READONLY 0x85
#define PART_WRITEONLY 0x86
#define PART_NO_ACCESS 0x87
/* ms*/
#define TIMEOUT                 300
#define NAND_CACHE_RW

#define NAND_IO_RESPONSE_TEST
static unsigned int dragonboard_test_flag;
struct secblc_op_t {
	int item;
	unsigned char *buf;
	unsigned int len;
};

struct burn_param_t {
	void *buffer;
	long length;
};

#ifdef CONFIG_COMPAT

struct secblc_op_t32 {
	int item;
	compat_uptr_t buf;
	unsigned int len;
};

struct burn_param_t32 {
	compat_uptr_t buffer;
	compat_size_t length;
};

struct hd_geometry32 {
	unsigned char heads;
	unsigned char sectors;
	unsigned short cylinders;
	compat_size_t start;
};

#endif

/*****************************************************************************/
extern int get_nand_secure_storage_max_item(void);
extern int nand_secure_storage_read(int item, unsigned char *buf,
				    unsigned int len);
extern int nand_secure_storage_write(int item, unsigned char *buf,
				     unsigned int len);

extern int NAND_ReadBoot0(unsigned int length, void *buf);
extern int NAND_ReadBoot1(unsigned int length, void *buf);
extern int NAND_BurnBoot0(unsigned int length, void *buf);
extern int NAND_BurnBoot1(unsigned int length, void *buf);

extern struct _nand_info *p_nand_info;

extern int add_nand(struct nand_blk_ops *tr,
		    struct _nand_phy_partition *phy_partition);
extern int add_nand_for_dragonboard_test(struct nand_blk_ops *tr);
extern int remove_nand(struct nand_blk_ops *tr);
extern int nand_flush(struct nand_blk_dev *dev);
extern struct _nand_phy_partition *get_head_phy_partition_from_nand_info(struct
									 _nand_info
									 *nand_info);
extern struct _nand_phy_partition *get_next_phy_partition(struct
							  _nand_phy_partition
							  *phy_partition);

/*****************************************************************************/

DEFINE_SEMAPHORE(nand_mutex);

unsigned char volatile IS_IDLE = 1;
static int nand_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd,
		      unsigned long arg);
#ifdef CONFIG_COMPAT
static int nand_compat_ioctl(struct block_device *bdev, fmode_t mode,
			     unsigned int cmd, unsigned long arg);
#endif
long max_r_io_response = 1;
long max_w_io_response = 1;

int debug_data;

struct timeval tpstart, tpend;
long timeuse;

/* print flags by name */
#if 0
const char *rq_flag_bit_names[] = {
	"REQ_WRITE",		/* not set, read. set, write */
	"REQ_FAILFAST_DEV",	/* no driver retries of device errors */
	"REQ_FAILFAST_TRANSPORT", /* no driver retries of transport errors */
	"REQ_FAILFAST_DRIVER",	/* no driver retries of driver errors */
	"REQ_SYNC",		/* request is sync (sync write or read) */
	"REQ_META",		/* metadata io request */
	"REQ_DISCARD",		/* request to discard sectors */
	"REQ_NOIDLE",		/* don't anticipate more IO after this one */
	"REQ_RAHEAD",		/* read ahead, can fail anytime *//* bio only flags */
	"REQ_THROTTLED",	/* This bio has already been subjected to * throttling rules. Don't do it again. */
	"REQ_SORTED",		/* elevator knows about this request */
	"REQ_SOFTBARRIER",	/* may not be passed by ioscheduler */
	"REQ_FUA",		/* forced unit access */
	"REQ_NOMERGE",		/* don't touch this for merging */
	"REQ_STARTED",		/* drive already may have started this one */
	"REQ_DONTPREP",		/* don't call prep for this one */
	"REQ_QUEUED",		/* uses queueing */
	"REQ_ELVPRIV",		/* elevator private data attached */
	"REQ_FAILED",		/* set if the request failed */
	"REQ_QUIET",		/* don't worry about errors */
	"REQ_PREEMPT",		/* set for "ide_preempt" requests */
	"REQ_ALLOCED",		/* request came from our alloc pool */
	"REQ_COPY_USER",	/* contains copies of user pages */
	"REQ_FLUSH",		/* request for cache flush */
	"REQ_FLUSH_SEQ",	/* request for flush sequence */
	"REQ_IO_STAT",		/* account I/O stat */
	"REQ_MIXED_MERGE",	/* merge of different types, fail separately */
	"REQ_SECURE",		/* secure discard (used with __REQ_DISCARD) */
	"REQ_NR_BITS",		/* stops here */
};
#endif

#if 0
void print_rq_flags(int flags)
{
	int i;
	uint32_t j;
	j = 1;
	NAND_Print_DBG("rq:");
	for (i = 0; i < 32; i++) {
		if (flags & j)
			NAND_Print_DBG("%s ", rq_flag_bit_names[i]);
		j = j << 1;
	}
	NAND_Print_DBG("\n");
}
#endif

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void start_time(int data)
{
#ifdef NAND_IO_RESPONSE_TEST

	if (debug_data != data)
		return;

	do_gettimeofday(&tpstart);

#endif
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int end_time(int data, int time, int par)
{
#ifdef NAND_IO_RESPONSE_TEST

	if (debug_data != data)
		return -1;

	do_gettimeofday(&tpend);
	timeuse =
	    1000 * (tpend.tv_sec - tpstart.tv_sec) * 1000 + (tpend.tv_usec -
							     tpstart.tv_usec);
	if (timeuse > time) {
		nand_dbg_err("%ld %d\n", timeuse, par);
		return 1;
	}
#endif
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int do_blktrans_request(struct nand_blk_ops *tr,
			       struct nand_blk_dev *dev, struct request *req)
{
	int ret = 0;
	unsigned int block, nsect;
	char *buf;

	struct _nand_dev *nand_dev;
	nand_dev = (struct _nand_dev *)dev->priv;

	block = blk_rq_pos(req) << 9 >> tr->blkshift;
	nsect = blk_rq_cur_bytes(req) >> tr->blkshift;

	buf = req->buffer;

	if (req->cmd_type != REQ_TYPE_FS) {
		nand_dbg_err(KERN_NOTICE "not type fs\n");
		return -EIO;
	}

	if (blk_rq_pos(req) + blk_rq_cur_sectors(req) >
	    get_capacity(req->rq_disk)) {
		nand_dbg_err(KERN_NOTICE "over capacity\n");
		return -EIO;
	}

	if (req->cmd_flags & REQ_DISCARD) {
		/*nand_dev->discard(nand_dev, block, nsect);*/
		goto request_exit;
	}

	switch (rq_data_dir(req)) {
	case READ:
		if (debug_data == 10) {
			nand_dbg_err("read_addr: %d nsect: %d buf: %p\n", block,
				     nsect, buf);
		}

		nand_dev->read_data(nand_dev, block, nsect, buf);
		rq_flush_dcache_pages(req);
		ret = 0;
		goto request_exit;

	case WRITE:
		rq_flush_dcache_pages(req);

		if (debug_data == 10) {
			nand_dbg_err("write_addr: %d nsect: %d buf: %p\n",
				     block, nsect, buf);
		}
		nand_dev->write_data(nand_dev, block, nsect, buf);

		ret = 0;

		goto request_exit;
	default:
		nand_dbg_err(KERN_NOTICE "Unknown request %u\n",
			     rq_data_dir(req));
		ret = -EIO;
		goto request_exit;
	}

request_exit:
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int mtd_blktrans_thread(void *arg)
{
	struct nand_blk_ops *tr = arg;
	struct request_queue *rq = tr->rq;
	struct request *req = NULL;
	struct nand_blk_dev *dev;
	int background_done = 0;

	spin_lock_irq(rq->queue_lock);

	while (!kthread_should_stop()) {
		int res;

		tr->bg_stop = false;
		if (!req) {
			req = blk_fetch_request(rq);
			if (!req) {
				set_current_state(TASK_INTERRUPTIBLE);
				if (kthread_should_stop())
					set_current_state(TASK_RUNNING);

				spin_unlock_irq(rq->queue_lock);
				tr->rq_null++;
				schedule();
				spin_lock_irq(rq->queue_lock);
				continue;
			}
		}

		dev = req->rq_disk->private_data;

		spin_unlock_irq(rq->queue_lock);
		tr->rq_null = 0;
		mutex_lock(&dev->lock);
		res = do_blktrans_request(tr, dev, req);
		mutex_unlock(&dev->lock);

		spin_lock_irq(rq->queue_lock);

		if (!__blk_end_request_cur(req, res))
			req = NULL;

		background_done = 0;
	}

	if (req)
		__blk_end_request_all(req, -EIO);

	spin_unlock_irq(rq->queue_lock);

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void mtd_blktrans_request(struct request_queue *rq)
{
	struct nand_blk_ops *nandr;
	struct request *req = NULL;

	nandr = rq->queuedata;

	if (!nandr)
		while ((req = blk_fetch_request(rq)) != NULL)
			__blk_end_request_all(req, -ENODEV);
	else {
		nandr->bg_stop = true;
		wake_up_process(nandr->thread);
	}
}

static void null_for_dragonboard(struct request_queue *rq)
{
	return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_open(struct block_device *bdev, fmode_t mode)
{
	struct nand_blk_dev *dev;
	struct nand_blk_ops *nandr;
	int ret = -ENODEV;

	dev = bdev->bd_disk->private_data;
	nandr = dev->nandr;

	if (!try_module_get(nandr->owner))
		goto out;

	ret = 0;
	if (nandr->open) {
		ret = nandr->open(dev);
		if (ret)
out:
			module_put(nandr->owner);
	}
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static void nand_release(struct gendisk *disk, fmode_t mode)
{
	struct nand_blk_dev *dev;
	struct nand_blk_ops *nandr;
	int ret = 0;

	dev = disk->private_data;
	nandr = dev->nandr;
	if (nandr->release)
		ret = nandr->release(dev);

	if (!ret)
		module_put(nandr->owner);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
#define DISABLE_WRITE           _IO('V', 0)
#define ENABLE_WRITE            _IO('V', 1)
#define DISABLE_READ            _IO('V', 2)
#define ENABLE_READ             _IO('V', 3)
#define DRAGON_BOARD_TEST       _IO('V', 55)
#define BLKREADBOOT0            _IO('v', 125)
#define BLKREADBOOT1            _IO('v', 126)
#define BLKBURNBOOT0            _IO('v', 127)
#define BLKBURNBOOT1            _IO('v', 128)
#define SECBLK_READ				_IO('V', 20)
#define SECBLK_WRITE			_IO('V', 21)
#define SECBLK_IOCTL			_IO('V', 22)

static int nand_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd,
		      unsigned long arg)
{
	struct nand_blk_dev *dev = bdev->bd_disk->private_data;
	struct nand_blk_ops *nandr = dev->nandr;
	struct burn_param_t *burn_param;
	struct secblc_op_t *sec_op;
	int ret = 0;
	unsigned char *buf_secure = NULL;

	burn_param = (struct burn_param_t *)arg;

	switch (cmd) {
	case BLKFLSBUF:
		if (nandr->flush)
			return nandr->flush(dev);
		/* The core code did the work, we had nothing to do. */
		return 0;

	case HDIO_GETGEO:
		if (nandr->getgeo) {
			struct hd_geometry g;
			int ret;

			memset(&g, 0, sizeof(g));
			ret = nandr->getgeo(dev, &g);
			if (ret)
				return ret;
			nand_dbg_err("HDIO_GETGEO called!\n");
			g.start = get_start_sect(bdev);
			if (copy_to_user((void __user *)arg, &g, sizeof(g)))
				return -EFAULT;

			return 0;
		}
		return 0;
	case ENABLE_WRITE:
		nand_dbg_err("enable write!\n");
		dev->disable_access = 0;
		dev->readonly = 0;
		set_disk_ro(dev->disk, 0);
		return 0;

	case DISABLE_WRITE:
		nand_dbg_err("disable write!\n");
		dev->readonly = 1;
		set_disk_ro(dev->disk, 1);
		return 0;

	case ENABLE_READ:
		nand_dbg_err("enable read!\n");
		dev->disable_access = 0;
		dev->writeonly = 0;
		return 0;

	case DISABLE_READ:
		nand_dbg_err("disable read!\n");
		dev->writeonly = 1;
		return 0;

	case BLKREADBOOT0:
		nand_dbg_err("start BLKREADBOOT0...\n");
		down(&nandr->nand_ops_mutex);
		IS_IDLE = 0;
		ret = NAND_ReadBoot0(burn_param->length, burn_param->buffer);
		if (ret != 0)
			nand_dbg_err("BLKREADBOOT0 failed\n");
		up(&(nandr->nand_ops_mutex));
		nand_dbg_err("do BLKREADBOOT0!\n");
		IS_IDLE = 1;
		return ret;

	case BLKREADBOOT1:
		nand_dbg_err("start BLKREADBOOT1...\n");
		down(&nandr->nand_ops_mutex);
		IS_IDLE = 0;
		ret = NAND_ReadBoot1(burn_param->length, burn_param->buffer);
		if (ret != 0)
			nand_dbg_err("BLKREADBOOT1 failed\n");
		up(&(nandr->nand_ops_mutex));
		nand_dbg_err("do BLKREADBOOT1!\n");
		IS_IDLE = 1;
		return ret;

	case BLKBURNBOOT0:
		nand_dbg_err("start BLKBURNBOOT0...\n");
		down(&nandr->nand_ops_mutex);
		IS_IDLE = 0;
		ret = NAND_BurnBoot0(burn_param->length, burn_param->buffer);
		if (ret != 0)
			nand_dbg_err("BLKBURNBOOT0 failed\n");
		up(&(nandr->nand_ops_mutex));
		nand_dbg_err("do BLKBURNBOOT0!\n");
		IS_IDLE = 1;
		return ret;

	case BLKBURNBOOT1:

		nand_dbg_err("start BLKBURNBOOT1...\n");
		down(&nandr->nand_ops_mutex);
		IS_IDLE = 0;
		ret = NAND_BurnBoot1(burn_param->length, burn_param->buffer);
		if (ret != 0)
			nand_dbg_err("BLKBURNBOOT1 failed\n");
		up(&(nandr->nand_ops_mutex));
		nand_dbg_err("do BLKBURNBOOT1!\n");
		IS_IDLE = 1;
		return ret;

	case SECBLK_READ:

		nand_dbg_err("start secure read ...\n");
		down(&nandr->nand_ops_mutex);
		IS_IDLE = 0;
		sec_op = (struct secblc_op_t *)arg;
		buf_secure = kmalloc(sec_op->len, GFP_KERNEL);
		if (buf_secure == NULL) {
			nand_dbg_err("buf_secure malloc fail!\n");
			return -1;
		}
		ret =
		    nand_secure_storage_read(sec_op->item, buf_secure,
					     sec_op->len);
		if (copy_to_user(sec_op->buf, buf_secure, sec_op->len))
			ret = -EFAULT;
		kfree(buf_secure);
		up(&(nandr->nand_ops_mutex));
		IS_IDLE = 1;
		return ret;

	case SECBLK_WRITE:

		nand_dbg_err("start secure write ...\n");
		down(&nandr->nand_ops_mutex);
		IS_IDLE = 0;
		sec_op = (struct secblc_op_t *)arg;
		buf_secure = kmalloc(sec_op->len, GFP_KERNEL);
		if (buf_secure == NULL) {
			nand_dbg_err("buf_secure malloc fail!\n");
			return -1;
		}
		if (copy_from_user
		    (buf_secure, (const void *)sec_op->buf, sec_op->len))
			ret = -EFAULT;
		ret =
		    nand_secure_storage_write(sec_op->item, buf_secure,
					      sec_op->len);
		kfree(buf_secure);
		up(&(nandr->nand_ops_mutex));
		IS_IDLE = 1;
		return ret;

	case SECBLK_IOCTL:

		nand_dbg_err("start secure get item...\n");
		down(&nandr->nand_ops_mutex);
		IS_IDLE = 0;
		ret = get_nand_secure_storage_max_item();
		up(&(nandr->nand_ops_mutex));
		IS_IDLE = 1;
		return ret;

	case DRAGON_BOARD_TEST:

		nand_dbg_err("start dragonborad test...\n");
		down(&(nandr->nand_ops_mutex));
		IS_IDLE = 0;
		ret = NAND_DragonboardTest();
		up(&(nandr->nand_ops_mutex));
		IS_IDLE = 1;
		return ret;

	default:
		nand_dbg_err("unknow cmd 0x%x!\n", cmd);
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
int nand_readboot_compat(struct block_device *bdev, fmode_t mode,
			  unsigned int cmd, struct burn_param_t32 __user *arg)
{
	struct burn_param_t32 burn_param32;
	struct burn_param_t __user *burn_param;
	int ret;

	if (copy_from_user(&burn_param32, arg, sizeof(burn_param32)))
		return -EFAULT;

	burn_param = compat_alloc_user_space(sizeof(*burn_param));
	if (!access_ok(VERIFY_WRITE, burn_param, sizeof(*burn_param)))
		return -EFAULT;

	if (put_user(compat_ptr(burn_param32.buffer), &burn_param->buffer) ||
	    put_user(burn_param32.length, &burn_param->length))
		return -EFAULT;

	ret = nand_ioctl(bdev, mode, cmd, (unsigned long)burn_param);
	if (ret < 0)
		return ret;

	if (copy_in_user(&arg->buffer, &burn_param->buffer, sizeof(arg->buffer)) ||
	    copy_in_user(&arg->length, &burn_param->length, sizeof(arg->length)))
		return -EFAULT;

	return 0;
}

int nand_burnboot_compat(struct block_device *bdev, fmode_t mode,
			  unsigned int cmd, struct burn_param_t32 __user *arg)
{
	struct burn_param_t32 burn_param32;
	struct burn_param_t __user *burn_param;
	int ret;

	if (copy_from_user(&burn_param32, arg, sizeof(burn_param32)))
		return -EFAULT;

	burn_param = compat_alloc_user_space(sizeof(*burn_param));
	if (!access_ok(VERIFY_WRITE, burn_param, sizeof(*burn_param)))
		return -EFAULT;

	if (put_user(compat_ptr(burn_param32.buffer), &burn_param->buffer) ||
	    put_user(burn_param32.length, &burn_param->length))
		return -EFAULT;

	ret = nand_ioctl(bdev, mode, cmd, (unsigned long)burn_param);
	if (ret < 0)
		return ret;

	return 0;
}

int nand_drangonboard_compat(struct block_device *bdev, fmode_t mode,
			     unsigned int cmd, unsigned long __user *arg)
{
	int ret;

	ret = nand_ioctl(bdev, mode, cmd, (unsigned long)arg);
	if (ret < 0)
		return ret;

	return 0;
}

int nand_securestorage_compat(struct block_device *bdev, fmode_t mode,
			      unsigned int cmd,
			      struct secblc_op_t32 __user *arg)
{
	struct secblc_op_t32 secblc_op32;
	struct secblc_op_t __user *secblc_op;
	int ret;

	if (copy_from_user(&secblc_op32, arg, sizeof(secblc_op32)))
		return -EFAULT;

	secblc_op = compat_alloc_user_space(sizeof(*secblc_op));
	if (!access_ok(VERIFY_WRITE, secblc_op, sizeof(*secblc_op)))
		return -EFAULT;

	if (put_user(secblc_op32.item, &secblc_op->item) ||
	    put_user(compat_ptr(secblc_op32.buf), &secblc_op->buf) ||
	    put_user(secblc_op32.len, &secblc_op->len))
		return -EFAULT;

	ret = nand_ioctl(bdev, mode, cmd, (unsigned long)secblc_op);
	if (ret < 0)
		return ret;

	if (copy_in_user(&arg->item, &secblc_op->item, sizeof(arg->item)) ||
	    copy_in_user(&arg->len, &secblc_op->len, sizeof(arg->len)))
		return -EFAULT;

	return 0;
}

int nand_getgeo_compat(struct block_device *bdev, fmode_t mode,
		       unsigned int cmd, struct hd_geometry32 __user *arg)
{
	struct hd_geometry32 getgeo32;
	struct hd_geometry __user *getgeo;
	int ret;

	if (copy_from_user(&getgeo32, arg, sizeof(getgeo32)))
		return -EFAULT;

	getgeo = compat_alloc_user_space(sizeof(*getgeo));
	if (!access_ok(VERIFY_WRITE, getgeo, sizeof(*getgeo)))
		return -EFAULT;

	if (put_user(getgeo32.heads, &getgeo->heads) ||
	    put_user(getgeo32.sectors, &getgeo->sectors) ||
	    put_user(getgeo32.cylinders, &getgeo->cylinders) ||
	    put_user(getgeo32.start, &getgeo->start))
		return -EFAULT;

	ret = nand_ioctl(bdev, mode, cmd, (unsigned long)getgeo);
	if (ret < 0)
		return ret;

	if (copy_in_user(&arg->heads, &getgeo->heads, sizeof(arg->heads)) ||
	    copy_in_user(&arg->sectors, &getgeo->sectors, sizeof(arg->sectors))
	    || copy_in_user(&arg->cylinders, &getgeo->cylinders,
			    sizeof(arg->cylinders))
	    || copy_in_user(&arg->start, &getgeo->start, sizeof(arg->start)))
		return -EFAULT;

	return 0;
}

static int nand_compat_ioctl(struct block_device *bdev, fmode_t mode,
			     unsigned int cmd, unsigned long arg)
{
	nand_dbg_err("start nand_compat_ioctl\n");
	switch (cmd) {
	case HDIO_GETGEO:
		return nand_getgeo_compat(bdev, mode, cmd, compat_ptr(arg));

	case BLKREADBOOT0:
	case BLKREADBOOT1:
		return nand_readboot_compat(bdev, mode, cmd, compat_ptr(arg));

	case BLKBURNBOOT0:
	case BLKBURNBOOT1:
		return nand_burnboot_compat(bdev, mode, cmd, compat_ptr(arg));

	case DRAGON_BOARD_TEST:
		return nand_drangonboard_compat(bdev, mode, cmd,
						compat_ptr(arg));

	case SECBLK_READ:
	case SECBLK_WRITE:
		return nand_securestorage_compat(bdev, mode, cmd,
						 compat_ptr(arg));

	default:
		return nand_ioctl(bdev, mode, cmd, arg);
	}
}
#endif

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/

const struct block_device_operations nand_blktrans_ops = {
	.owner = THIS_MODULE,
	.open = nand_open,
	.release = nand_release,
	.ioctl = nand_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = nand_compat_ioctl,
#endif

};

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_blk_open(struct nand_blk_dev *dev)
{
#if 0
	nand_dbg_err("nand_blk_open!\n");
	mutex_lock(&dev->lock);
	nand_dbg_err("nand_open ok!\n");

	kref_get(&dev->ref);

	mutex_unlock(&dev->lock);
#endif
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_blk_release(struct nand_blk_dev *dev)
{
	int error = 0;
	struct _nand_dev *nand_dev = (struct _nand_dev *)dev->priv;
	if (dragonboard_test_flag == 0) {
		error = nand_dev->flush_write_cache(nand_dev, 0xffff);
		return error;
	} else
		return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int del_nand_blktrans_dev(struct nand_blk_dev *dev)
{
/*
	if (!down_trylock(&nand_mutex)) {
		up(&nand_mutex);
		BUG();
    }
	blk_cleanup_queue(dev->rq);
	kthread_stop(dev->thread);
*/
	list_del(&dev->list);
	dev->disk->queue = NULL;
	del_gendisk(dev->disk);
	put_disk(dev->disk);

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nand_getgeo(struct nand_blk_dev *dev, struct hd_geometry *geo)
{
	geo->heads = dev->heads;
	geo->sectors = dev->sectors;
	geo->cylinders = dev->cylinders;

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
struct nand_blk_ops mytr = {
	.name = "nand",
	.major = 93,
	.minorbits = 4,
	.blksize = 512,
	.blkshift = 9,
	.open = nand_blk_open,
	.release = nand_blk_release,
	.getgeo = nand_getgeo,
	.add_dev = add_nand,
	.add_dev_test = add_nand_for_dragonboard_test,
	.remove_dev = remove_nand,
	.flush = nand_flush,
	.owner = THIS_MODULE,
};

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void set_part_mod(char *name, int cmd)
{
	struct file *filp = NULL;
	filp = filp_open(name, O_RDWR, 0);
	filp->f_dentry->d_inode->i_bdev->bd_disk->fops->ioctl(filp->f_dentry->
							      d_inode->i_bdev,
							      0, cmd, 0);
	filp_close(filp, current->files);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int add_nand_blktrans_dev(struct nand_blk_dev *dev)
{
	struct nand_blk_ops *tr = dev->nandr;
	struct list_head *this;
	struct gendisk *gd;
	unsigned long temp;
	int ret = -ENOMEM;

	int last_devnum = -1;

	dev->cylinders = 1024;
	dev->heads = 16;

	temp = dev->cylinders * dev->heads;
	dev->sectors = (dev->size) / temp;
	if ((dev->size) % temp) {
		dev->sectors++;
		temp = dev->cylinders * dev->sectors;
		dev->heads = (dev->size) / temp;

		if ((dev->size) % temp) {
			dev->heads++;
			temp = dev->heads * dev->sectors;
			dev->cylinders = (dev->size) / temp;
		}
	}

	if (!down_trylock(&nand_mutex)) {
		up(&nand_mutex);
		BUG();
	}

	list_for_each(this, &tr->devs) {
		struct nand_blk_dev *tmpdev =
		    list_entry(this, struct nand_blk_dev, list);
		if (dev->devnum == -1) {
			/* Use first free number */
			if (tmpdev->devnum != last_devnum + 1) {
				/* Found a free devnum. Plug it in here */
				dev->devnum = last_devnum + 1;
				list_add_tail(&dev->list, &tmpdev->list);
				goto added;
			}
		} else if (tmpdev->devnum == dev->devnum) {
			/* Required number taken */
			nand_dbg_err("\nerror00\n");
			return -EBUSY;
		} else if (tmpdev->devnum > dev->devnum) {
			/* Required number was free */
			list_add_tail(&dev->list, &tmpdev->list);
			goto added;
		}
		last_devnum = tmpdev->devnum;
	}
	if (dev->devnum == -1)
		dev->devnum = last_devnum + 1;

	if ((dev->devnum << tr->minorbits) > 256) {
		nand_dbg_err("\nerror00000\n");
		return -EBUSY;
	}

	list_add_tail(&dev->list, &tr->devs);

added:
	gd = alloc_disk(1 << tr->minorbits);
	if (!gd) {
		list_del(&dev->list);
		goto error2;
	}

	gd->major = tr->major;
	gd->first_minor = (dev->devnum) << tr->minorbits;
	gd->fops = &nand_blktrans_ops;

	snprintf(gd->disk_name, sizeof(gd->disk_name), "%s%c", tr->name,
		 (tr->minorbits ? 'a' : '0') + dev->devnum);
	/* 2.5 has capacity in units of 512 bytes while still
	   having BLOCK_SIZE_BITS set to 10. Just to keep us amused. */
	set_capacity(gd, dev->size);

	gd->private_data = dev;
	dev->disk = gd;
	tr->rq->bypass_depth++;
	gd->queue = tr->rq;

	dev->disable_access = 0;
	dev->readonly = 0;
	dev->writeonly = 0;
	mutex_init(&dev->lock);
	add_disk(gd);

	return 0;

error2:
	nand_dbg_err("\nerror2\n");
	list_del(&dev->list);
	return ret;
}

int add_nand_blktrans_dev_for_dragonboard(struct nand_blk_dev *dev)
{
	struct nand_blk_ops *tr = dev->nandr;
	struct gendisk *gd;
	int ret = -ENOMEM;

	gd = alloc_disk(1);
	if (!gd) {
		list_del(&dev->list);
		goto error2;
	}

	gd->major = tr->major;
	gd->first_minor = 0;
	gd->fops = &nand_blktrans_ops;

	snprintf(gd->disk_name, sizeof(gd->disk_name),
		"%s%c", tr->name, (1 ? 'a' : '0') + dev->devnum);
	set_capacity(gd, 512);

	gd->private_data = dev;
	dev->disk = gd;
	gd->queue = tr->rq;

	dev->disable_access = 0;
	dev->readonly = 0;
	dev->writeonly = 0;

	mutex_init(&dev->lock);

    /* Create the request queue */
	spin_lock_init(&tr->queue_lock);
	tr->rq = blk_init_queue(null_for_dragonboard, &tr->queue_lock);
	if (!tr->rq)
		goto error3;

	tr->rq->queuedata = dev;
	blk_queue_logical_block_size(tr->rq, tr->blksize);

	gd->queue = tr->rq;
	add_disk(gd);

	return 0;

error3:
	nand_dbg_err("\nerror3\n");
	put_disk(dev->disk);
error2:
	nand_dbg_err("\nerror2\n");
	list_del(&dev->list);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_blk_register(struct nand_blk_ops *tr)
{
	int ret;
	struct _nand_phy_partition *phy_partition;

	down(&nand_mutex);

	ret = register_blkdev(tr->major, tr->name);
	if (ret) {
		nand_dbg_err("\nfaild to register blk device\n");
		up(&nand_mutex);
		return -1;
	}

	spin_lock_init(&tr->queue_lock);
	init_completion(&tr->thread_exit);
	init_waitqueue_head(&tr->thread_wq);
	sema_init(&tr->nand_ops_mutex, 1);

	tr->rq = blk_init_queue(mtd_blktrans_request, &tr->queue_lock);
	if (!tr->rq) {
		unregister_blkdev(tr->major, tr->name);
		up(&nand_mutex);
		return -1;
	}
/*
		ret = elevator_change(tr->rq, "noop");
		if(ret){
			blk_cleanup_queue(tr->rq);
			return ret;
		}
*/
	tr->rq->queuedata = tr;
	blk_queue_logical_block_size(tr->rq, tr->blksize);
	blk_queue_max_hw_sectors(tr->rq, 128);

	tr->thread = kthread_run(mtd_blktrans_thread, tr, "%s", tr->name);
	if (IS_ERR(tr->thread)) {
		ret = PTR_ERR(tr->thread);
		blk_cleanup_queue(tr->rq);
		unregister_blkdev(tr->major, tr->name);
		up(&nand_mutex);
		return ret;
	}

	queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, tr->rq);
	tr->rq->limits.max_discard_sectors = UINT_MAX;

	INIT_LIST_HEAD(&tr->devs);
	tr->nftl_blk_head.nftl_blk_next = NULL;
	tr->nand_dev_head.nand_dev_next = NULL;

	phy_partition = get_head_phy_partition_from_nand_info(p_nand_info);

	while (phy_partition != NULL) {
		tr->add_dev(tr, phy_partition);
		phy_partition = get_next_phy_partition(phy_partition);
	}
	tr->rq->bypass_depth--;
	up(&nand_mutex);

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void nand_blk_unregister(struct nand_blk_ops *tr)
{

	down(&nand_mutex);
	/* Clean up the kernel thread */
	tr->quit = 1;
	wake_up(&tr->thread_wq);
	wait_for_completion(&tr->thread_exit);

	/* Remove it from the list of active majors */
	tr->remove_dev(tr);

	unregister_blkdev(tr->major, tr->name);
	blk_cleanup_queue(tr->rq);
	up(&nand_mutex);

	if (!list_empty(&tr->devs))
		BUG();
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
/*
int cal_partoff_within_disk(char *name, struct inode *i)
{
	struct gendisk *gd = i->i_bdev->bd_disk;
	int current_minor = MINOR(i->i_bdev->bd_dev);
	int index = current_minor & ((1 << mytr.minorbits) - 1);
	if (!index)
		return 0;
	return (gd->part_tbl->part[index - 1]->start_sect);
}
*/
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int init_blklayer(void)
{
#if 0
	script_item_u good_block_ratio_flag;
	script_item_value_type_e type;

	type =
	    script_get_item("nand0_para", "good_block_ratio",
			    &good_block_ratio_flag);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
		nand_dbg_inf("nand type err!\n");
#endif
	return nand_blk_register(&mytr);
}

int init_blklayer_for_dragonboard(void)
{
	int ret;
	struct nand_blk_ops *tr;

	tr = &mytr;

	dragonboard_test_flag = 1;

	down(&nand_mutex);

	ret = register_blkdev(tr->major, tr->name);
	if (ret) {
		nand_dbg_err("\nfaild to register blk device\n");
		up(&nand_mutex);
		return -1;
	}

	init_completion(&tr->thread_exit);
	init_waitqueue_head(&tr->thread_wq);
	sema_init(&tr->nand_ops_mutex, 1);

	INIT_LIST_HEAD(&tr->devs);
	tr->nftl_blk_head.nftl_blk_next = NULL;
	tr->nand_dev_head.nand_dev_next = NULL;

	tr->add_dev_test(tr);

	up(&nand_mutex);

	return 0;

}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void exit_blklayer(void)
{
	nand_blk_unregister(&mytr);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int __init nand_drv_init(void)
{
	return init_blklayer();
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void __exit nand_drv_exit(void)
{
	exit_blklayer();

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("nand flash groups");
MODULE_DESCRIPTION("Generic NAND flash driver code");