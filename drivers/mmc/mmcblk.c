/* 
 * This file contains a generic in memory block driver that will be hooked
 * to the mmc sub-system
 */
#include <minix/syslib.h>
#include <minix/driver.h>
#include <minix/blockdriver.h>

#include <sys/ioc_disk.h> /* disk IOCTL's */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <signal.h>

#include "mmclog.h"

#define SUBPARTITION_PER_PARTITION 4 /* 4 sub partitions per partition */
#define PARTITONS_PER_DISK 4         /* 4 partitions per disk */
#define MINOR_PER_DISK  1            /* one additional minor to point to */

/**
 * We can have multiple MMC host controller present on the hardware. The MINIX
 * approach to handle this is to run a driver for each instance. Every driver
 * will therefore be stated with an "instance" id and the rest of the code here
 * will assume a single host controller to be present.
 *
 * The SD specification allows multiple cards to be attached to a single host
 * controller using the same lines. I recommend reading SD Specifications Part 1
 * Physical layer Simplified Specification chapter 3 about SD Memory Card system
 * concepts if you want to get a better understanding of this.
 *
 * In practice an MMC host will usually have a single slot attached to it and that
 * Slot may or may not contain a card. On sudden card removal we might want to
 * keep track of the last inserted card and we might therefore at later stage
 * add an additional "last_card" attribute to the card.
 *
 * The following diagram shows the structure that will be used to modulate the
 * hardware written in umlwiki  syntax.
 *
 * [/host/
 *   +instance:int] 1 --- 0..4 [ /slot/
 *                                +card_detect:func ] 1 --- 0..1 [ /card/ ]
 *                                 `
 */

#define MAX_SDLOTS 4

struct mmc_host;

//TODO Add more modes like INACTIVE STATE and such
#define SD_MODE_CARD_IDENTIFICATION 1
#define SD_MODE_DATA_TRANSFER_MODE 2

/* struct representing an SD card */
struct sd_card
{
	/* pointer back to the sd slot for convenience */
	struct sd_slot *slot;

	/* Card registers */
	uint32_t cid[4]; /* Card Identification */
	uint32_t rca; /* Relative card address */
	uint32_t dsr; /* Driver stage register */
	uint32_t csd[4]; /* Card specific data */
	uint32_t scr[2]; /* SD configuration */
	uint32_t ocr; /* Operation conditions */
	uint32_t ssr[5]; /* SD Status */
	uint32_t csr; /* Card status */

	/* drive state: deaf, initialized, dead */
	unsigned state;

	/* MINIX/block driver related things */
	int open_ct; /* in-use count */
	/* 1 disks + 4 partitions and 16 possible sub partitions */
	struct device part[MINOR_PER_DISK + PARTITONS_PER_DISK];
	struct device subpart[PARTITONS_PER_DISK * SUBPARTITION_PER_PARTITION];
};

/* struct representing an sd slot */
struct sd_slot
{
	/* pointer back to the host for convenience */
	struct mmc_host *host;
	//unsigned index;

	unsigned state;
	struct sd_card card;
};

struct mmc_command;

/* struct for the host controller */
struct mmc_host
{
	/* MMC host configuration */
	int (*host_set_instance)(struct mmc_host *host, int instance);
	/* MMC host configuration */
	int (*host_init)(struct mmc_host *host);
	/* Host controller reset */
	int (*host_reset)(struct mmc_host *host);
	/* Card detection (binary yes/no) */
	int (*card_detect)(struct sd_slot* slot);
	/* Perform card detection e.g. card type */
	struct sd_card* (*card_initialize)(struct sd_slot* slot);
	/* Release the card */
	int (*card_release)(struct sd_card* card);
	/* Command execution */
	int (*send_cmd)(struct sd_card* card, struct mmc_command *);

	/* up to 4 slots with ... 4 cards*/
	struct sd_slot slot[MAX_SDLOTS];
};

/* struct representing an mmc command */
struct mmc_command
{

};

/*
 * Define a structure to be used for logging
 */
static struct mmclog log= {
		.name = "mmc_driver",
		.log_level = LEVEL_TRACE,
		.log_func = default_log };

/* while developing..  don;t rely on proper headers*/
#include "mmchost.c"

static struct mmc_host host;

#define SLOT_STATE_INITIAL 0x01u


static struct sd_slot * get_slot(dev_t minor);

static void initialize_structures()
{
	/* Initialize the basic data structures host slots and cards */
	int i;

	host.host_set_instance =mmchs_host_set_instance;
	host.host_init =mmchs_host_init;
	host.host_reset = mmchs_host_reset;
	host.card_detect = mmchs_card_detect;
	host.card_initialize = mmchs_card_initialize;
	host.card_release =mmchs_card_release;

	/* initialize data structures */
	for (i =0; i < MAX_SDLOTS ; i++){
		//@TODO set initial card and slot state
		host.slot[i].host = &host;
		host.slot[i].card.slot = & host.slot[i];
	}

}
/* Prototypes for the block device */
static int block_open(dev_t minor, int access);
static int block_close(dev_t minor);
static int block_transfer(dev_t minor,
		int do_write,
		u64_t position,
		endpoint_t endpt,
		iovec_t *iov,
		unsigned int nr_req,
		int flags);
static int block_ioctl(dev_t minor,
		unsigned int request,
		endpoint_t endpt,
		cp_grant_id_t grant);

/* System even handling */
static void sef_local_startup();
static int block_system_event_cb(int type, sef_init_info_t *info);
static void block_signal_handler_cb(int signo);

static struct device *block_part(dev_t minor);

/* Entry points for the BLOCK driver. */
static struct blockdriver mmc_driver= {
		BLOCKDRIVER_TYPE_DISK,/* handle partition requests */
		block_open, /* open or mount */
		block_close, /*  on a close */
		block_transfer, /* does the I/O */
		block_ioctl, /* ioclt's */
		NULL, /* no need to clean up (yet)*/
		block_part, /* return partition information */
		NULL, /* no geometry */
		NULL, /* no interrupt processing */
		NULL, /* no alarm processing */
		NULL, /* no processing of other messages */
		NULL /* no threading support */
};

int apply_env()
{
	/* apply the env setting passed to this driver
	 * parameters accepted
	 * log_level=[0-4] (NONE,WARNING,INFO,DEBUG|TRACE)
	 * instance=[0-3] instance/bus number to use for this driver
	 *
	 * Passing these arguments is done when starting the driver using
	 * the service command in the following way
	 *
	 * service up /sbin/mmcblk -args "log_level=2 instance=1"
	 **/
	long v;

	/* Initialize the verbosity level. */
	v= 0;
	if (env_parse("log_level", "d", 0, &v, LEVEL_NONE, LEVEL_TRACE) == EP_SET) {
		mmc_log_debug(&log, "Setting verbosity level to %d\n", v);
		log.log_level= v;
	}

	/* Find out which driver instance we are. */
	v= 0;
	env_parse("instance", "d", 0, &v, 0, 3);
	if (host.host_set_instance(&host, v)) {
		mmc_log_warn(&log, "Failed to set mmc instance to  %d\n", v);
		return 1; /* NOT OK */
	}
	return OK;
}

int main(int argc, char **argv)
{
	/* Set and apply the environment */
	env_setargs(argc, argv);

	initialize_structures();
	if (apply_env()) {
		mmc_log_warn(&log, "Failed while applying environment settings\n");
		return EXIT_FAILURE;
	}mmc_log_info(&log, "Initializing the MMC block device\n");
	if (host.host_init(&host)) {
		mmc_log_warn(&log, "Failed to initialize the host controller\n");
		return EXIT_FAILURE;
	}

	sef_local_startup();
	blockdriver_task(&mmc_driver);
	return EXIT_SUCCESS;
}
;

/*===========================================================================*
 *                    block_open                                             *
 *===========================================================================*/
static int block_open(dev_t minor, int access)
{
	struct sd_slot *slot;
	slot= get_slot(minor);
	if (!slot) {
		mmc_log_debug(&log, "Not handling open on non existing slot\n");
		return EIO;
	}

	assert(slot->host != NULL);

	if (!slot->host->card_detect(slot)) {
		mmc_log_debug(&log, "No card inserted in the SD slot\n");
		return EIO;
	}

	/* If we are already open just increase the open count and return */
	if (slot->card.state == SD_MODE_DATA_TRANSFER_MODE) {
		assert(slot->card.open_ct >= 0);
		slot->card.open_ct++;
		mmc_log_trace(&log, "increased open count to %d\n", slot->card.open_ct);
		return OK;
	}

	/* We did not have an sd-card inserted so we are going to probe for it */
	mmc_log_debug(&log, "First open on (%d)\n", minor);
	if (!host.card_initialize(slot)) {
		//TODO set card state to INVALID until removed?
		return EIO;
	}
	slot->card.open_ct++;
	assert(slot->card.open_ct == 1);
	return OK;
}

/*===========================================================================*
 *                    block_close                                            *
 *===========================================================================*/
static int block_close(dev_t minor)
{
	struct sd_slot *slot;

	slot= get_slot(minor);
	if (!slot) {
		mmc_log_debug(&log, "Not handling open on non existing slot\n");
		return EIO;
	}

	/* if we arrived here we expect a card to be present, we will need do
	 * deal with removal later */
	assert(slot->host != NULL);
	assert(slot->card.open_ct >=1);

	/* If this is not the last open count simply decrease the counter and return */
	if (slot->card.open_ct > 1) {
		slot->card.open_ct--;
		mmc_log_trace(&log, "decreased open count to %d\n",
				slot->card.open_ct);
		return OK;
	}

	assert(slot->card.open_ct == 1);
	mmc_log_debug(&log, "freeing the block device as it is no longer used\n");

	/* release the card as check the open_ct should be 0 */
	slot->host->card_release(&slot->card);
	assert(slot->card.open_ct == 0);
	return OK;
}

/*===========================================================================*
 *                    block_transfer                                         *
 *===========================================================================*/
static int block_transfer(dev_t minor /* minor device number */,
		int do_write /* read or write? */,
		u64_t position /* offset on device to read or write */,
		endpoint_t endpt /* process doing the request */,
		iovec_t *iov /* pointer to read or write request vector */,
		unsigned int nr_req /* length of request vector */,
		int flags /* transfer flags */
		)
{
	unsigned long counter;
	iovec_t *ciov; /* Current IO Vector */
	struct device * dev; /* The device used */
	vir_bytes io_size; /*  Size to read/write to/from the iov */
	vir_bytes io_offset; /* Size to read/write to/from the iov */
	vir_bytes bytes_written;
	int r;

	/* get the current "device" geometry */
	dev= block_part(minor);

	bytes_written= 0;

	/* are we trying to start reading past the end */
	if (position > dev->dv_size) {
		return ENXIO;
	};

	if (nr_req != 1) {
		mmc_log_warn(&log, "code untested on multiple requests please "
		" report success or failure (%d)\n", nr_req);
	}

	ciov= iov;
	for (counter= 0; counter < nr_req; counter++) {
		assert(ciov != NULL);

		/* Assume we are to transfer the amount of data given in the
		 * input/output vector but ensure we are not doing i/o past 
		 * our own boundaries 
		 */
		io_size= ciov->iov_size;
		io_offset= position + bytes_written;
		/* Check we are not reading/writing past the end */
		if (position + bytes_written + io_size > dev->dv_size) {
			io_size= dev->dv_size - (position + bytes_written);
		};

		mmc_log_trace(
				&log,
				"I/O request(%d/%d) iov(base,size,iosize,"
				"offset)=(%d,%d,%d,%d)\n",
				counter, nr_req,
				ciov->iov_addr, ciov->iov_size, io_size, io_offset);
		if (do_write) {
			/* Read io_size bytes from i/o vector starting at 0 
			 * and write it to out buffer at the correct offset */
			r= sys_safecopyfrom(endpt, ciov->iov_addr, 0 /* offset */,
					(vir_bytes) dummy_data + io_offset, io_size);
		} else {
			/* Read io_size bytes from our data at the correct 
			 * offset and write it to the output buffer at 0 */
			r= sys_safecopyto(endpt, ciov->iov_addr, 0 /* offset */,
					(vir_bytes) dummy_data + io_offset, io_size);
		}
		if (r != OK) {
			/* use _SIGN to reverse the signedness */
			mmc_log_warn(
					&log,
					"I/O %s error: %s iov(base,size)=(%d,%d)"
					" at offset=%d\n",
					(do_write)?"write":"read", strerror(_SIGN r), ciov->iov_addr, ciov->iov_size, io_offset);
			return EIO;
		}

		ciov++;
		bytes_written+= io_size;
	}
	return bytes_written;
}

/*===========================================================================*
 *				block_ioctl				                                     *
 *===========================================================================*/
static int block_ioctl(dev_t minor,
		unsigned int request,
		endpoint_t endpt,
		cp_grant_id_t grant)
{
	/*  IOCTL handling  */
	struct sd_slot *slot;
	mmc_log_trace(&log, "enter (minor,request,endpoint,grant)=(%d,%d,%d)\n",
			minor, request, endpt, grant);

	slot= get_slot(minor);
	if (slot) {
		mmc_log_warn(&log, "Doing ioctl on non existing block device(%d)\n",
				minor);
		return EINVAL;
	}

	switch (request) {
		case DIOCOPENCT:
			//TODO: add a check for card validity */
			/*  return the current open count */
			return sys_safecopyto(endpt, grant, 0,
					(vir_bytes) slot->card.open_ct, sizeof(vir_bytes));
		case DIOCFLUSH:
			/* No need to flush but some devices like movinands require 500
			 * ms inactivity*/
			return OK;
	}

	return EINVAL;
}

/*===========================================================================*
 *                    block_part                                             *
 *===========================================================================*/
static struct device *block_part(dev_t minor)
{
	/*
	 * Reuse the existing MINIX major/minor partitioning scheme.
	 * - 8 drives
	 * - 5 devices per drive allowing direct access to the disk and up to 4
	 *   partitions (IBM style partitioning without extended partitions)
	 * - 4 Minix style sub partitions per partitions
	 *
	 *  The partition scheme goes like this:
	 *  | minor | disk    | partition    | sub  | name   |
	 *  | 0     | 0       | 0            |      | d0     | first disk
	 *  | 1     | 0       | 1            |      | d0p0   | first disk , first partition
	 *  | 1     | 0       | 2            |      | d0p1   | first disk , second partition
	 *  | 2     | 0       | 3            |      | d0p2   | ..
	 *  | 3     | 0       | 4            |      | d0p3   |
	 *  | 4     | 1       | 0            |      | d1     | second disk
	 *  | 5     | 1       | 1            |      | d1p0   | second disk , first partition
	 *  | ...   | minor/8 | minor % 5    |      |        |
	 *  | 40    | 7       |  4           |      | d7p3   | eights disk , fourth partition
	 *  | gap   | --      | --           |      | --     | not used. next start 128 are sub partitions
	 *  | 128   | 0       | 1            | 1    | d0p0s0 | first first , first disk , first sub
	 *  | 129   | 0       | 1            | 2    | d0p0s0 | first first , first disk , first sub
	 *  | ...   | ...     | ...          |      |        |
	 *  | 255   | 7       | 4            | 4    | d7p3s3 |
	 *
	 *  returns the device in question if persent of NULL if no such minor device exists
	 */
	struct device *dev;
	struct sd_slot *slot;

	dev= NULL;
	slot= get_slot(minor);
	if (!slot) {
		mmc_log_warn(&log,
				"Device information requested for non existing partition "
				"minor(%d)\n", minor);
		return NULL;
	}

	if (!slot->host->card_detect(slot)) {
		mmc_log_warn(&log, "Device information requested from empty slot(%d)\n",
				minor);
		return NULL;
	}

	if (minor < 5) {
		/* we are talking about the first disk */
		dev= &slot->card.part[minor];
		mmc_log_trace(&log,
				"returning partition(%d) (base,size)=(0x%08x,0x%08x)\n",
				minor, dev->dv_base, dev->dv_size);

	} else if (minor >= 128 && minor <= 128 + 16) {
		/* sub partitions of the first disk we don't care about the rest */
		dev= &slot->card.subpart[minor - 128];
		mmc_log_trace(&log,
				"returning sub partition(%d) (base,size)=(0x%08x,0x%08x)\n",
				minor - 128, dev->dv_base, dev->dv_size);

	} else {
		mmc_log_warn(
				&log,
				"Device information requested for non existing partition minor(%d)\n",
				minor);
	}
	return dev;
}

/*===========================================================================*
 *                         sef_local_startup                                 *
 *===========================================================================*/
static void sef_local_startup()
{
	/*
	 * Register init callbacks. Use the same function for all event types
	 */
	sef_setcb_init_fresh(block_system_event_cb); /* Callback on a freh start */
	sef_setcb_init_lu(block_system_event_cb); /* Callback on a Live Update */
	sef_setcb_init_restart(block_system_event_cb); /* Callback on a restart */

	/* Register signal callbacks. */
	sef_setcb_signal_handler(block_signal_handler_cb);

	/* SEF startup */
	sef_startup();
}

/*===========================================================================*
 *                         block_system_event_cb                              *
 *===========================================================================*/
static int block_system_event_cb(int type, sef_init_info_t *info)
{
	/**
	 * Callbacks for the System event framework as registered in sef_local_startup
	 */
	switch (type) {
		case SEF_INIT_FRESH:
			mmc_log_info(&log, "System event framework fresh start\n");
			break;

		case SEF_INIT_LU:
			/* Restore the state. post update */
			mmc_log_info(&log, "System event framework live update\n");
			break;

		case SEF_INIT_RESTART:
			mmc_log_info(&log, "System event framework post restart\n");
			break;
	}
	return OK;
}

/*===========================================================================*
 *                         block_signal_handler_cb                           *
 *===========================================================================*/
static void block_signal_handler_cb(int signo)
{
	mmc_log_debug(&log, "System event framework signal handler sig(%d)\n",
			signo);
	/* Only check for termination signal, ignore anything else. */
	if (signo != SIGTERM)
		return;
	//FIXME shutdown
	exit(0);
}

static struct sd_slot * get_slot(dev_t minor)
{
	/*
	 * Get an sd_slot based on the minor number.
	 *
	 * This driver only supports a single card at the time. Also as
	 * we are following the major/minor scheme of other driver we
	 * must return a slot for all minors on disk 0 these are  0-5
	 * for the disk and 4 main partitions and
	 * number 128 till 144 for sub partitions.
	 */
	if (minor < 5) {
		/* we are talking about the first disk and that is all we support*/
		return &host.slot[0];
	} else if (minor >= 128 && minor <= 128 + 16) {
		/* a minor from the first disk */
		return &host.slot[0];
	} else {
		mmc_log_trace( &log,
				"Device information requested for non existing partition "
				"minor(%d)\n", minor);
		return NULL;
	}
}
