/* 
 * This file contains a generic in memory block driver that will be hooked to the mmc 
 * sub-system
 */
#include <minix/syslib.h>
#include <minix/driver.h>
#include <minix/blockdriver.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define DUMMY_SIZE_IN_BLOCKS 1000
static char dummy_data[SECTOR_SIZE * DUMMY_SIZE_IN_BLOCKS];

static struct device device_geometry = {
	.dv_base = 0,
	.dv_size = SECTOR_SIZE * DUMMY_SIZE_IN_BLOCKS
};

/* Prototypes for the block device */
static int block_open(dev_t minor, int access);
static int block_close(dev_t minor);
static int block_transfer(dev_t minor, int do_write, u64_t position,
        endpoint_t endpt, iovec_t *iov, unsigned int nr_req, int flags);
static struct device *block_part(dev_t minor);

/* Entry points for the BLOCK driver. */
static struct blockdriver mmc_driver = {
  BLOCKDRIVER_TYPE_DISK,/* handle partition requests */
  block_open,         /* open or mount */
  block_close,        /* nothing on a close */
  block_transfer,     /* do the I/O */
  NULL,               /* No ioclt's */
  NULL,               /* no need to clean up (yet)*/
  block_part,         /* return partition information */
  NULL,               /* no geometry */
  NULL,               /* no interrupt processing */
  NULL,               /* no alarm processing */
  NULL,               /* no processing of other messages */
  NULL                /* no threading support */
};


int main(int argc, char **argv)
{
  /* SEF startup */
  sef_startup();
  printf("Initializing the MMC block device\n");	
  blockdriver_task(&mmc_driver);

  return EXIT_SUCCESS;
};

/*===========================================================================*
 *                    block_open                                             *
 *===========================================================================*/
static int block_open(dev_t minor, int access)
{
  /* open on the memory device nothing special to do here */
  //TODO: increase open count ? */
  return OK;
}

/*===========================================================================*
 *                    block_close                                            *
 *===========================================================================*/
static int block_close(dev_t minor)
{
  /* handle close on the memory device nothing special to do here */
  //TODO: decrease the open count ? */
  return OK;
}

/*===========================================================================*
 *                    block_transfer                                         *
 *===========================================================================*/
static int block_transfer(
  dev_t minor,          /* minor device number */
  int do_write,         /* read or write? */
  u64_t position,       /* offset on device to read or write */
  endpoint_t endpt,     /* process doing the request */
  iovec_t *iov,         /* pointer to read or write request vector */
  unsigned int nr_req,  /* length of request vector */ 
  int flags             /* transfer flags */
  )
{
	unsigned long counter;
	iovec_t *ciov;          /* Current IO Vector */
	struct device * dev;    /* The device used */
	vir_bytes io_size;      /* size to read/write to/from the iov */
	vir_bytes bytes_written; 
	int r;

	/* get the current "device" geomerty */
	dev = block_part(minor);

	bytes_written =0;

	/* are we trying to start reading past the end */
	if (position > dev->dv_size) { return ENXIO; }; 

	if (nr_req != 1){
		printf("Number of request > 1 \n");
	}
	ciov = iov;
	for( counter = 0 ; counter < nr_req ; counter ++){
		assert(ciov != NULL);

		/* Assume we are to tranfer the amount of data given in the 
		 * input/output vector but ensure we are not doing i/o past 
		 * our own boundaries 
		 */
		io_size = ciov->iov_size;

		/* check we are not reading/writing past the end */
		if (position + bytes_written + io_size > dev->dv_size ) {
			io_size = dev->dv_size - ( position + bytes_written );
		};
		if(do_write){
			/* Read io_size bytes from i/o vector starting at 0 
			 * and write it to out buffer at the correct offset */
			r=sys_safecopyfrom(endpt, ciov->iov_addr,0 /* offset */, 
			                      (vir_bytes) dummy_data + position 
					      + bytes_written, io_size);
		} else {
			/* Read io_size bytes from our data at the correct 
			 * offset and write it to the output buffer at 0 */
			r=sys_safecopyto(endpt, ciov->iov_addr,0 /* offset */,
			                    (vir_bytes) dummy_data  + position 
					    + bytes_written , io_size);
		}
		if (r != OK){
			panic("I/O %s failed: %d", (do_write)?"write":"read", r);
		}

		ciov++;		
		bytes_written += io_size;
	}
	return bytes_written;
}

/*===========================================================================*
 *                    block_part                                             *
 *===========================================================================*/
static struct device *block_part(dev_t minor)
{
	return  &device_geometry;
}
