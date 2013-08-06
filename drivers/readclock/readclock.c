/* readclock - manipulate the hardware real time clock */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <minix/type.h>
#include <minix/const.h>
#include <minix/callnr.h>
#include <minix/log.h>
#include <minix/syslib.h>
#include <minix/sysutil.h>
#include <minix/com.h>
#include <minix/type.h>
#include <minix/safecopies.h>
#include <sys/svrctl.h>

#include "readclock.h"

static struct log log = {
	.name = "readclock",
	.log_level = LEVEL_INFO,
	.log_func = default_log
};

/* functions for transfering struct tm to/from this driver and calling proc. */
static int fetch_t(endpoint_t who_e, vir_bytes rtcdev_tm, struct tm *t);
static int store_t(endpoint_t who_e, vir_bytes rtcdev_tm, struct tm *t);

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t * info);

int
main(int argc, char **argv)
{
	int r;
	endpoint_t caller;
	struct tm t;
	message m;
	int ipc_status, reply_status;

	env_setargs(argc, argv);
	sef_local_startup();

	while (TRUE) {

		/* Receive Message */
		r = sef_receive_status(ANY, &m, &ipc_status);
		if (r != OK) {
			log_warn(&log, "sef_receive_status() failed\n");
			continue;
		}

		if (is_ipc_notify(ipc_status)) {

			/* Do not reply to notifications. */
			continue;
		}

		caller = m.m_source;

		log_debug(&log, "Got message 0x%x from 0x%x\n", m.m_type, caller);

		switch (m.m_type) {
		case RTCDEV_GET_TIME:
			/* Any user can read the time */
			reply_status = arch_get_time(&t, m.RTCDEV_FLAGS);
			if (reply_status != OK) {
				break;
			}

			/* write results back to calling process */
			reply_status =
			    store_t(caller, (vir_bytes) m.RTCDEV_TM, &t);
			break;

		case RTCDEV_SET_TIME:
			/* Only super user is allowed to set the time */
			if (getnuid(caller) == SUPER_USER) {
				/* read time from calling process */
				reply_status =
				    fetch_t(caller, (vir_bytes) m.RTCDEV_TM,
				    &t);
				if (reply_status != OK) {
					break;
				}

				reply_status =
				    arch_set_time(&t, m.RTCDEV_FLAGS);
			} else {
				reply_status = EPERM;
			}
			break;

		case RTCDEV_PWR_OFF:
			/* Only PM is allowed to set the power off time */
			if (caller == PM_PROC_NR) {
				reply_status = arch_pwr_off();
			} else {
				reply_status = EPERM;
			}
			break;

		default:
			/* Unrecognized call */
			reply_status = EINVAL;
			break;
		}

		/* Send Reply */
		m.m_type = RTCDEV_REPLY;
		m.RTCDEV_STATUS = reply_status;

		log_debug(&log, "Sending Reply");

		r = sendnb(caller, &m);
		if (r != OK) {
			log_warn(&log, "sendnb() failed\n");
			continue;
		}
	}

	arch_exit();
	return 0;
}

static int
sef_cb_init(int type, sef_init_info_t * UNUSED(info))
{
	int r;

	if (type == SEF_INIT_LU) {
		/* Restore the state. */
		arch_lu_state_restore();
	}

	r = arch_init();
	if (r != OK) {
		return r;
	}

	if (type != SEF_INIT_LU) {
		/* Some RTCs need to do a driver announcement */
		arch_announce();
	}

	return OK;
}

static void
sef_local_startup()
{
	/*
	 * Register init callbacks. Use the same function for all event types
	 */
	sef_setcb_init_fresh(sef_cb_init);
	sef_setcb_init_lu(sef_cb_init);
	sef_setcb_init_restart(sef_cb_init);

	/*
	 * Register live update callbacks.
	 */
	/* Agree to update immediately when LU is requested in a valid state. */
	sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
	/* Support live update starting from any standard state. */
	sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
	/* Register a custom routine to save the state. */
	sef_setcb_lu_state_save(arch_sef_cb_lu_state_save);

	/* Let SEF perform startup. */
	sef_startup();
}

int
bcd_to_dec(int n)
{
	return ((n >> 4) & 0x0F) * 10 + (n & 0x0F);
}

int
dec_to_bcd(int n)
{
	return ((n / 10) << 4) | (n % 10);
}

static int
fetch_t(endpoint_t who_e, vir_bytes rtcdev_tm, struct tm *t)
{
	return sys_datacopy(who_e, rtcdev_tm, SELF, (vir_bytes) t,
	    sizeof(struct tm));
}

static int
store_t(endpoint_t who_e, vir_bytes rtcdev_tm, struct tm *t)
{
	return sys_datacopy(SELF, (vir_bytes) t, who_e, rtcdev_tm,
	    sizeof(struct tm));
}