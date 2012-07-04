/******************************************************************************/
/* libvinetic.h                                                               */
/******************************************************************************/

#ifndef __LIBVINETIC__H__
#define __LIBVINETIC_H__

#include <sys/types.h>

#include <limits.h>

#include "polygator/vinetic-ioctl.h"
#include "polygator/vinetic-def.h"

struct vinetic_context {

	char dev_path[PATH_MAX];
	int dev_fd;

	char pram_path[PATH_MAX];
	char dram_path[PATH_MAX];

	char alm_dsp_ab_path[PATH_MAX];
	char alm_dsp_cd_path[PATH_MAX];

	char cram_path[PATH_MAX];

	int error;
	int errorline;

	u_int16_t revision;

	struct vin_eop_edsp_sw_version_register edsp_sw_version_register;
};

extern void vin_init(struct vinetic_context *ctx, const char *fmt, ...);
extern void vin_set_pram(struct vinetic_context *ctx, const char *fmt, ...);
extern void vin_set_dram(struct vinetic_context *ctx, const char *fmt, ...);
extern void vin_set_alm_dsp_ab(struct vinetic_context *ctx, const char *fmt, ...);
extern void vin_set_alm_dsp_cd(struct vinetic_context *ctx, const char *fmt, ...);
extern void vin_set_cram(struct vinetic_context *ctx, const char *fmt, ...);
extern int vin_open(struct vinetic_context *ctx);
extern void vin_close(struct vinetic_context *ctx);
extern int vin_reset(struct vinetic_context *ctx);
extern int vin_reset_rdyq(struct vinetic_context *ctx);
extern int vin_flush_mbox(struct vinetic_context *ctx);
extern int vin_is_not_ready(struct vinetic_context *ctx);
extern u_int16_t vin_read_dia(struct vinetic_context *ctx);

extern int vin_resync(struct vinetic_context *ctx);
extern int vin_cerr_acknowledge(struct vinetic_context *ctx);

extern int vin_poll_set(struct vinetic_context *ctx, int poll);

extern char *vin_dev_name(struct vinetic_context *ctx);
extern char *vin_error_str(struct vinetic_context *ctx);

#define VIN_REV_13 0x2442
#define VIN_REV_14 0x2484
extern char *vin_revision_str(struct vinetic_context *ctx);


extern int vin_read_fw_version(struct vinetic_context *ctx);

extern ssize_t vin_write(struct vinetic_context *ctx, const void *buf, size_t count);

extern ssize_t vin_read(struct vinetic_context *ctx, union vin_cmd cmd, void *buf, size_t count);

extern u_int16_t vin_phi_revision(struct vinetic_context *ctx);
extern u_int16_t vin_phi_checksum(struct vinetic_context *ctx);
extern int vin_phi_disable_interrupt(struct vinetic_context *ctx);

extern int vin_download_edsp_firmware(struct vinetic_context *ctx);

extern int vin_download_alm_dsp(struct vinetic_context *ctx, char *path);
extern int vin_jump_alm_dsp(struct vinetic_context *ctx, unsigned int chan);

extern int vin_download_cram(struct vinetic_context *ctx, unsigned int chan, char *path);

extern int vin_write_sop_generic(struct vinetic_context *ctx, unsigned int chan, u_int16_t offset, u_int16_t data);
extern int vin_alm_channel_test_set(struct vinetic_context *ctx, unsigned int chan, int en);
extern int vin_alm_channel_dcctl_pram_set(struct vinetic_context *ctx, unsigned int chan, int pram_dcc);

extern int vin_coder_channel_jb_statistic_reset(struct vinetic_context *ctx, unsigned int chan);

extern int vin_set_endian_mode(struct vinetic_context *ctx, int mode);

#define vin_set_little_endian_mode(_ctx) \
	({ \
		int res = vin_set_endian_mode(_ctx, VIN_LITTLE_ENDIAN); \
		res; \
	})

#define VIN_GAINDB_MIN -24.08
#define VIN_GAINDB_MAX 23.95

extern double vin_gainem_to_gaindb(u_int8_t em);
extern u_int8_t vin_gaindb_to_gainem(double g);

#endif //__LIBVINETIC_H__

/******************************************************************************/
/* end of libvinetic.h                                                        */
/******************************************************************************/
