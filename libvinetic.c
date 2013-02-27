/******************************************************************************/
/* libvinetic.c                                                               */
/******************************************************************************/

#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>

#include "libvinetic/libvinetic.h"
#include "libvinetic/version.h"

static const struct vin_eop_coder_channel_configuration_rtp_support default_eop_coder_channel_configuration_rtp_support = {
	.ssrc_hw = 0x0000, // 1
	.ssrc_lw = 0x0000, // 2
	.seq_nr = 0x0000, // 3
	.pt_00011 = 0x00, // 4 - 8880
	.sid_00011 = 1,
	.pt_00010 = 0x08,
	.sid_00010 = 1,
	.pt_00101 = 0x71, // 5 - F0F1
	.sid_00101 = 1,
	.pt_00100 = 0x70,
	.sid_00100 = 1,
	.pt_00111 = 0x72, // 6 - 82F2
	.sid_00111 = 1,
	.pt_00110 = 0x02,
	.sid_00110 = 1,
	.pt_01001 = 0x7F, // 7 - FFFF
	.sid_01001 = 1,
	.pt_01000 = 0x7F,
	.sid_01000 = 1,
	.pt_01011 = 0x7F, // 8 - FFFF
	.sid_01011 = 1,
	.pt_01010 = 0x7F,
	.sid_01010 = 1,
	.pt_01101 = 0x7F, // 9 - FFFF
	.sid_01101 = 1,
	.pt_01100 = 0x7F,
	.sid_01100 = 1,
	.pt_01111 = 0x7F, // 10 - FFFF
	.sid_01111 = 1,
	.pt_01110 = 0x7F,
	.sid_01110 = 1,
	.pt_10001 = 0x7F, // 11 - FFFF
	.sid_10001 = 1,
	.pt_10000 = 0x7F,
	.sid_10000 = 1,
	.pt_10011 = 0x60, // 12 - 1260
	.sid_10011 = 0,
	.pt_10010 = 0x12,
	.sid_10010= 0,
	.pt_10101 = 0x7F, // 13 - FFFF
	.sid_10101 = 1,
	.pt_10100 = 0x7F,
	.sid_10100 = 1,
	.pt_10111 = 0x7F, // 14 - FFFF
	.sid_10111 = 1,
	.pt_10110 = 0x7F,
	.sid_10110 = 1,
	.pt_11001 = 0x7F, // 15 - FFFF
	.sid_11001 = 1,
	.pt_11000 = 0x7F,
	.sid_11000 = 1,
	.pt_11011 = 0x7F, // 16 - FFFF
	.sid_11011 = 1,
	.pt_11010 = 0x7F,
	.sid_11010 = 1,
	.pt_11101 = 0x04, // 17 - 0404
	.sid_11101 = 0,
	.pt_11100 = 0x04,
	.sid_11100 = 0,
	.pt_11111 = 0x5C, // 18 - FF5C
	.sid_11111 = 0,
	.pt_11110 = 0x7F,
	.sid_11110 = 1,
};

static void vin_message_stack_init(struct vinetic_context *ctx)
{
	if (ctx->message_stack_buf) {
		free(ctx->message_stack_buf);
	}
	ctx->message_stack_buf = NULL;
	ctx->message_stack_len = 0;
	ctx->message_stack_ptr = NULL;
	ctx->message_stack_out = NULL;
}

void vin_message_stack_printf(struct vinetic_context *ctx, const char *format, ...)
{
	va_list ap;

	ctx->message_stack_buf = realloc(ctx->message_stack_buf, ctx->message_stack_len + 1024);

	if (ctx->message_stack_buf) {
		va_start(ap, format);
		ctx->message_stack_len += vsprintf(ctx->message_stack_buf + ctx->message_stack_len, format, ap);
		va_end(ap);

		ctx->message_stack_len += sprintf(ctx->message_stack_buf + ctx->message_stack_len, "\r\n");

		ctx->message_stack_buf = realloc(ctx->message_stack_buf, ctx->message_stack_len + 1);
	}

	ctx->message_stack_ptr = ctx->message_stack_buf;
	ctx->message_stack_out = ctx->message_stack_buf;
}

const char *vin_message_stack_check_line(struct vinetic_context *ctx)
{
	const char *res = NULL;

	while (ctx->message_stack_ptr) {
		res = strsep(&ctx->message_stack_ptr, "\r\n");
		if (res && strlen(res)) {
			break;
		} else {
			res = NULL;
		}
	}

	if (!res) {
		vin_message_stack_init(ctx);
	} else {
		ctx->message_stack_out = res;
	}

	return res;
}

const char *vin_message_stack_get_line(struct vinetic_context *ctx)
{
	return ctx->message_stack_out;
}

void vin_init(struct vinetic_context *ctx, const char *fmt, ...)
{
	size_t i;

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(ctx->dev_path, sizeof(ctx->dev_path), fmt, ap);
	va_end(ap);
	ctx->dev_fd = -1;

	ctx->dev_name = NULL;

	for (i=0; i<4; i++) {
		// SLIC's operation mode
		ctx->ali_opmode[i] = VIN_OP_MODE_PDHI;
		// default codec map
		memcpy(&ctx->eop_coder_channel_configuration_rtp_support[i],
				&default_eop_coder_channel_configuration_rtp_support,
				sizeof(struct vin_eop_coder_channel_configuration_rtp_support));
	}

	// init message stack
	vin_message_stack_init(ctx);
}

void vin_destroy(struct vinetic_context *ctx)
{
	vin_message_stack_init(ctx); // free message buffer
}

int vin_set_pram(struct vinetic_context *ctx, const char *fmt, ...)
{
	int res;
	struct stat buf;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(ctx->pram_path, sizeof(ctx->pram_path), fmt, ap);
	va_end(ap);

	if ((res = stat(ctx->pram_path, &buf)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s stat(%s) failed: %s", __LINE__, __PRETTY_FUNCTION__, ctx->pram_path, strerror(errno));
	}

	return res;
}

int vin_set_dram(struct vinetic_context *ctx, const char *fmt, ...)
{
	int res;
	struct stat buf;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(ctx->dram_path, sizeof(ctx->dram_path), fmt, ap);
	va_end(ap);

	if ((res = stat(ctx->dram_path, &buf)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s stat(%s) failed: %s", __LINE__, __PRETTY_FUNCTION__, ctx->dram_path, strerror(errno));
	}

	return res;
}

int vin_set_alm_dsp_ab(struct vinetic_context *ctx, const char *fmt, ...)
{
	int res;
	struct stat buf;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(ctx->alm_dsp_ab_path, sizeof(ctx->alm_dsp_ab_path), fmt, ap);
	va_end(ap);

	if ((res = stat(ctx->alm_dsp_ab_path, &buf)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s stat(%s) failed: %s", __LINE__, __PRETTY_FUNCTION__, ctx->alm_dsp_ab_path, strerror(errno));
	}

	return res;
}

int vin_set_alm_dsp_cd(struct vinetic_context *ctx, const char *fmt, ...)
{
	int res;
	struct stat buf;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(ctx->alm_dsp_cd_path, sizeof(ctx->alm_dsp_cd_path), fmt, ap);
	va_end(ap);

	if ((res = stat(ctx->alm_dsp_cd_path, &buf)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s stat(%s) failed: %s", __LINE__, __PRETTY_FUNCTION__, ctx->alm_dsp_cd_path, strerror(errno));
	}

	return res;
}

int vin_set_cram(struct vinetic_context *ctx, const char *fmt, ...)
{
	int res;
	struct stat buf;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(ctx->cram_path, sizeof(ctx->cram_path), fmt, ap);
	va_end(ap);

	if ((res = stat(ctx->cram_path, &buf)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s stat(%s) failed: %s", __LINE__, __PRETTY_FUNCTION__, ctx->cram_path, strerror(errno));
	}

	return res;
}

int vin_open(struct vinetic_context *ctx)
{
	if ((ctx->dev_fd = open(ctx->dev_path, O_RDWR)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s open(%s) failed: %s", __LINE__, __PRETTY_FUNCTION__, ctx->dev_path, strerror(errno));
	}
	return ctx->dev_fd;
}

void vin_close(struct vinetic_context *ctx)
{
	close(ctx->dev_fd);
	ctx->dev_fd = -1;
}

int vin_reset(struct vinetic_context *ctx)
{
	int res;

	if ((res = ioctl(ctx->dev_fd, VINETIC_RESET, NULL)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_RESET) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
	}

	return res;
}

int vin_reset_rdyq(struct vinetic_context *ctx)
{
	int res;

	if ((res = ioctl(ctx->dev_fd, VINETIC_RESET_RDYQ, NULL)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_RESET_RDYQ) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
	}

	return res;
}

int vin_flush_mbox(struct vinetic_context *ctx)
{
	int res;
	if ((res = ioctl(ctx->dev_fd, VINETIC_FLUSH_MBOX, NULL)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_FLUSH_MBOX) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
	}

	return res;
}

int vin_is_not_ready(struct vinetic_context *ctx)
{
	int not_ready;
	if (ioctl(ctx->dev_fd, VINETIC_GET_NOT_READY, &not_ready) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_GET_NOT_READY) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		not_ready = -1;
	}
	return not_ready;
}

u_int16_t vin_read_dia(struct vinetic_context *ctx)
{
	u_int16_t dia;
	if (ioctl(ctx->dev_fd, VINETIC_READ_DIA, &dia) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_READ_DIA) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		dia = 0xffff;
	}
	return dia;
}

int vin_resync(struct vinetic_context *ctx)
{
	int res;
	union vin_cmd_short cmd_short;

	// Re-SYNChronize PCM clock
	cmd_short.full = VIN_wRESYNC;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
	}

	return res;
}

int vin_cerr_acknowledge(struct vinetic_context *ctx)
{
	int res;
	off64_t lsres;
	union vin_cmd cmd;

	cmd.parts.first.bits.rw = VIN_WRITE;
	cmd.parts.first.bits.sc = VIN_SC_NO;
	cmd.parts.first.bits.bc = VIN_BC_NO;
	cmd.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd.parts.first.bits.res = 0;
	cmd.parts.first.bits.chan = 0;
	cmd.parts.second.eop.bits.mod = VIN_MOD_CONT;
	cmd.parts.second.eop.bits.ecmd  = VIN_EOP_CERR_ACK;
	cmd.parts.second.eop.bits.length = 0;
	if ((lsres = lseek64(ctx->dev_fd, cmd.full, SEEK_SET)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek64() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		res = (int)lsres;
		goto vin_cerr_acknowledge_end;
	}
	if ((res = write(ctx->dev_fd, &cmd.full, 0)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_cerr_acknowledge_end;
	}

vin_cerr_acknowledge_end:
	return res;
}

int vin_poll_set(struct vinetic_context *ctx, int poll)
{
	int res;
	
	if ((res = ioctl(ctx->dev_fd, VINETIC_SET_POLL, &poll)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_SET_POLL, %d) failed: %s", __LINE__, __PRETTY_FUNCTION__, poll, strerror(errno));
	}

	return res;
}

char *vin_get_dev_name(struct vinetic_context *ctx)
{
	if (ctx->dev_name && strlen(ctx->dev_name))
		return ctx->dev_name;
	else
		return "unknown";
}

void vin_set_dev_name(struct vinetic_context *ctx, char *name)
{
	if (name && strlen(name))
		ctx->dev_name = name;
	else
		ctx->dev_name = "unknown";
}

char *vin_revision_str(struct vinetic_context *ctx)
{
	switch (ctx->revision) {
		case VIN_REV_13: return "1.3";
		case VIN_REV_14: return "1.4";
		default: return "unknown";
	}
}

int vin_reset_status(struct vinetic_context *ctx)
{
	int res;

	if ((res = ioctl(ctx->dev_fd, VINETIC_RESET_STATUS, NULL)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd,ctx->dev_fd, VINETIC_RESET_STATUS, NULL) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
	}

	return res;
}

ssize_t vin_write(struct vinetic_context *ctx, size_t track_err, const void *buf, size_t count)
{
	ssize_t res;
	off64_t lsres;
	union vin_cmd cmd;
	u_int8_t *data = (u_int8_t *)buf;
	size_t length;
	union vin_cmd auxcmd;
	struct vin_read_bxsr bxsr;

	cmd.full = 0;
	if (count < 2) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s count=%lu to short", __LINE__, __PRETTY_FUNCTION__, (unsigned long int)count);
		res = -EINVAL;
		goto vin_write_end;
	} else if (count == 2) {
		memcpy(&cmd.parts.first.full, data, 2);
		length = 0;
	} else if(count == 4) {
		memcpy(&cmd.full, data, 4);
		length = 0;
	} else {
		memcpy(&cmd.full, data, 4);
		length = count - 4;
	}
#if 0
	// check mailbox status
	auxcmd.full = 0;
	auxcmd.parts.first.full = VIN_rBXSR;
	if ((lsrc = lseek64(ctx->dev_fd, auxcmd.full, SEEK_SET)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		rc = (ssize_t)lsrc;
		goto vin_write_end;
	}
	if ((rc = read(ctx->dev_fd, &bxsr, sizeof(struct vin_read_bxsr))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_write_end;
	}

	if (bxsr.bxsr2.bits.host_err || bxsr.bxsr2.bits.pibx_of || bxsr.bxsr2.bits.cibx_of) {
		// acknowledge error
		auxcmd.full = 0;
		auxcmd.parts.first.full = VIN_wPHIERR;
		if ((lsrc = lseek64(ctx->dev_fd, auxcmd.full, SEEK_SET)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			rc = (ssize_t)lsrc;
			goto vin_write_end;
		}
		if ((rc = write(ctx->dev_fd, &auxcmd.full, 0)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_write_end;
		}
		// wait for recovery
		usleep(500);
	}
#endif
	// base write command
	if ((lsres = lseek64(ctx->dev_fd, cmd.full, SEEK_SET)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek64() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		res = (ssize_t)lsres;
		goto vin_write_end;
	}
	if ((res = write(ctx->dev_fd, data+4, length)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_write_end;
	}
	if (track_err) {
		usleep(130);
	}
	// check mailbox status
	auxcmd.full = 0;
	auxcmd.parts.first.full = VIN_rBXSR;
	if ((lsres = lseek64(ctx->dev_fd, auxcmd.full, SEEK_SET)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek64() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		res = (ssize_t)lsres;
		goto vin_write_end;
	}
	if ((res = read(ctx->dev_fd, &bxsr, sizeof(struct vin_read_bxsr))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_write_end;
	}
	if (bxsr.bxsr1.bits.cerr) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s cerr=1", __LINE__, __PRETTY_FUNCTION__);
#if 0
		if (vin_cerr_acknowledge(ctx) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_write_end;
		}
#endif
		res = -1;
		goto vin_write_end;
	}

vin_write_end:
	return res;
}

ssize_t vin_read(struct vinetic_context *ctx, union vin_cmd cmd, void *buf, size_t count)
{
	ssize_t res;
	off64_t lsres;
	union vin_cmd auxcmd;
	struct vin_read_bxsr bxsr;

	// check mailbox status
	auxcmd.full = 0;
	auxcmd.parts.first.full = VIN_rBXSR;
	if ((lsres = lseek64(ctx->dev_fd, auxcmd.full, SEEK_SET)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek64() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		res = (ssize_t)lsres;
		goto vin_read_end;
	}
	if ((res = read(ctx->dev_fd, &bxsr, sizeof(struct vin_read_bxsr))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_read_end;
	}

	if (bxsr.bxsr2.bits.host_err || bxsr.bxsr2.bits.pibx_of || bxsr.bxsr2.bits.cibx_of) {
		// acknowledge error
		auxcmd.full = 0;
		auxcmd.parts.first.full = VIN_wPHIERR;
		if ((lsres = lseek64(ctx->dev_fd, auxcmd.full, SEEK_SET)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek64() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			res = (ssize_t)lsres;
			goto vin_read_end;
		}
		if ((res = write(ctx->dev_fd, &auxcmd.full, 0)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_read_end;
		}
		// wait for recovery
		usleep(500);
	}
	// base read command
	if ((lsres = lseek64(ctx->dev_fd, cmd.full, SEEK_SET)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek64() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		res = (ssize_t)lsres;
		goto vin_read_end;
	}
	if ((res = read(ctx->dev_fd, buf, count)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_read_end;
	}

vin_read_end:
	return res;
}

ssize_t vin_get_status(struct vinetic_context *ctx)
{
	ssize_t res;
	off64_t lsres;

	// read command
	if ((lsres = lseek64(ctx->dev_fd, 0xffffffff, SEEK_SET)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek64() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		res = (ssize_t)lsres;
		goto vin_get_status_end;
	}
	if ((res = read(ctx->dev_fd, &ctx->status, sizeof(struct vin_status_registers))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_get_status_end;
	}

vin_get_status_end:
	return res;
}

ssize_t vin_set_status_mask(struct vinetic_context *ctx)
{
	ssize_t res;
	off64_t lsres;

	// read command
	if ((lsres = lseek64(ctx->dev_fd, 0xffffffff, SEEK_SET)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek64() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		res = (ssize_t)lsres;
		goto vin_set_status_mask_end;
	}
	if ((res = write(ctx->dev_fd, &ctx->status_mask, sizeof(struct vin_status_registers))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_set_status_mask_end;
	}

vin_set_status_mask_end:
	return res;
}

void vin_status_monitor(struct vinetic_context *ctx)
{
	struct vin_status_registers status_changed;

	// get changes
	status_changed.sr.sre1_0.full = (ctx->status.sr.sre1_0.full ^ ctx->status_old.sr.sre1_0.full) & ctx->status_mask.sr.sre1_0.full;
	status_changed.sr.sre2_0.full = (ctx->status.sr.sre2_0.full ^ ctx->status_old.sr.sre2_0.full) & ctx->status_mask.sr.sre2_0.full;
	status_changed.sr.srs1_0.full = (ctx->status.sr.srs1_0.full ^ ctx->status_old.sr.srs1_0.full) & ctx->status_mask.sr.srs1_0.full;
	status_changed.sr.srs2_0.full = (ctx->status.sr.srs2_0.full ^ ctx->status_old.sr.srs2_0.full) & ctx->status_mask.sr.srs2_0.full;
	status_changed.sr.sre1_1.full = (ctx->status.sr.sre1_1.full ^ ctx->status_old.sr.sre1_1.full) & ctx->status_mask.sr.sre1_1.full;
	status_changed.sr.sre2_1.full = (ctx->status.sr.sre2_1.full ^ ctx->status_old.sr.sre2_1.full) & ctx->status_mask.sr.sre2_1.full;
	status_changed.sr.srs1_1.full = (ctx->status.sr.srs1_1.full ^ ctx->status_old.sr.srs1_1.full) & ctx->status_mask.sr.srs1_1.full;
	status_changed.sr.srs2_1.full = (ctx->status.sr.srs2_1.full ^ ctx->status_old.sr.srs2_1.full) & ctx->status_mask.sr.srs2_1.full;
	status_changed.sr.sre1_2.full = (ctx->status.sr.sre1_2.full ^ ctx->status_old.sr.sre1_2.full) & ctx->status_mask.sr.sre1_2.full;
	status_changed.sr.sre2_2.full = (ctx->status.sr.sre2_2.full ^ ctx->status_old.sr.sre2_2.full) & ctx->status_mask.sr.sre2_2.full;
	status_changed.sr.srs1_2.full = (ctx->status.sr.srs1_2.full ^ ctx->status_old.sr.srs1_2.full) & ctx->status_mask.sr.srs1_2.full;
	status_changed.sr.srs2_2.full = (ctx->status.sr.srs2_2.full ^ ctx->status_old.sr.srs2_2.full) & ctx->status_mask.sr.srs2_2.full;
	status_changed.sr.sre1_3.full = (ctx->status.sr.sre1_3.full ^ ctx->status_old.sr.sre1_3.full) & ctx->status_mask.sr.sre1_3.full;
	status_changed.sr.sre2_3.full = (ctx->status.sr.sre2_3.full ^ ctx->status_old.sr.sre2_3.full) & ctx->status_mask.sr.sre2_3.full;
	status_changed.sr.srs1_3.full = (ctx->status.sr.srs1_3.full ^ ctx->status_old.sr.srs1_3.full) & ctx->status_mask.sr.srs1_3.full;
	status_changed.sr.srs2_3.full = (ctx->status.sr.srs2_3.full ^ ctx->status_old.sr.srs2_3.full) & ctx->status_mask.sr.srs2_3.full;
	status_changed.sr.sre1_4.full = (ctx->status.sr.sre1_4.full ^ ctx->status_old.sr.sre1_4.full) & ctx->status_mask.sr.sre1_4.full;
	status_changed.sr.sre2_4.full = (ctx->status.sr.sre2_4.full ^ ctx->status_old.sr.sre2_4.full) & ctx->status_mask.sr.sre2_4.full;
	status_changed.sr.sre1_5.full = (ctx->status.sr.sre1_5.full ^ ctx->status_old.sr.sre1_5.full) & ctx->status_mask.sr.sre1_5.full;
	status_changed.sr.sre2_5.full = (ctx->status.sr.sre2_5.full ^ ctx->status_old.sr.sre2_5.full) & ctx->status_mask.sr.sre2_5.full;
	status_changed.sr.sre1_6.full = (ctx->status.sr.sre1_6.full ^ ctx->status_old.sr.sre1_6.full) & ctx->status_mask.sr.sre1_6.full;
	status_changed.sr.sre2_6.full = (ctx->status.sr.sre2_6.full ^ ctx->status_old.sr.sre2_6.full) & ctx->status_mask.sr.sre2_6.full;
	status_changed.sr.sre1_7.full = (ctx->status.sr.sre1_7.full ^ ctx->status_old.sr.sre1_7.full) & ctx->status_mask.sr.sre1_7.full;
	status_changed.sr.sre2_7.full = (ctx->status.sr.sre2_7.full ^ ctx->status_old.sr.sre2_7.full) & ctx->status_mask.sr.sre2_7.full;
	status_changed.hwsr.hwsr1.full = (ctx->status.hwsr.hwsr1.full ^ ctx->status_old.hwsr.hwsr1.full) & ctx->status_mask.hwsr.hwsr1.full;
	status_changed.hwsr.hwsr2.full = (ctx->status.hwsr.hwsr2.full ^ ctx->status_old.hwsr.hwsr2.full) & ctx->status_mask.hwsr.hwsr2.full;
	status_changed.bxsr.bxsr1.full = (ctx->status.bxsr.bxsr1.full ^ ctx->status_old.bxsr.bxsr1.full) & ctx->status_mask.bxsr.bxsr1.full;
	status_changed.bxsr.bxsr2.full = (ctx->status.bxsr.bxsr2.full ^ ctx->status_old.bxsr.bxsr2.full) & ctx->status_mask.bxsr.bxsr2.full;
	// store current status as old
	memcpy(&ctx->status_old, &ctx->status, sizeof(struct vin_status_registers));
	// handle status changes
	if (status_changed.sr.sre1_0.full) {
		;
	}
	if (status_changed.sr.sre2_0.full) {
		;
	}
	if (status_changed.sr.srs1_0.full) {
		if (status_changed.sr.srs1_0.bits.hook && ctx->vin_hook_handler[0].handler && ctx->vin_hook_handler[0].data) {
			ctx->vin_hook_handler[0].handler(ctx->vin_hook_handler[0].data, ctx->status.sr.srs1_0.bits.hook);
		}
	}
	if (status_changed.sr.srs2_0.full) {
		;
	}
	if (status_changed.sr.sre1_1.full) {
		;
	}
	if (status_changed.sr.sre2_1.full) {
		;
	}
	if (status_changed.sr.srs1_1.full) {
		if (status_changed.sr.srs1_1.bits.hook && ctx->vin_hook_handler[1].handler && ctx->vin_hook_handler[1].data) {
			ctx->vin_hook_handler[1].handler(ctx->vin_hook_handler[1].data, ctx->status.sr.srs1_1.bits.hook);
		}
	}
	if (status_changed.sr.srs2_1.full) {
		;
	}
	if (status_changed.sr.sre1_2.full) {
		;
	}
	if (status_changed.sr.sre2_2.full) {
		;
	}
	if (status_changed.sr.srs1_2.full) {
		if (status_changed.sr.srs1_2.bits.hook && ctx->vin_hook_handler[2].handler && ctx->vin_hook_handler[2].data) {
			ctx->vin_hook_handler[2].handler(ctx->vin_hook_handler[2].data, ctx->status.sr.srs1_2.bits.hook);
		}
	}
	if (status_changed.sr.srs2_2.full) {
		;
	}
	if (status_changed.sr.sre1_3.full) {
		;
	}
	if (status_changed.sr.sre2_3.full) {
		;
	}
	if (status_changed.sr.srs1_3.full) {
		if (status_changed.sr.srs1_3.bits.hook && ctx->vin_hook_handler[3].handler && ctx->vin_hook_handler[3].data) {
			ctx->vin_hook_handler[3].handler(ctx->vin_hook_handler[3].data, ctx->status.sr.srs1_3.bits.hook);
		}
	}
	if (status_changed.sr.srs2_3.full) {
		;
	}
	if (status_changed.sr.sre1_4.full) {
		;
	}
	if (status_changed.sr.sre2_4.full) {
		;
	}
	if (status_changed.sr.sre1_5.full) {
		;
	}
	if (status_changed.sr.sre2_5.full) {
		;
	}
	if (status_changed.sr.sre1_6.full) {
		;
	}
	if (status_changed.sr.sre2_6.full) {
		;
	}
	if (status_changed.sr.sre1_7.full) {
		;
	}
	if (status_changed.sr.sre2_7.full) {
		;
	}
	if (status_changed.hwsr.hwsr1.full) {
		;
	}
	if (status_changed.hwsr.hwsr2.full) {
		;
	}
	if (status_changed.bxsr.bxsr1.full) {
		;
	}
	if (status_changed.bxsr.bxsr2.full) {
		;
	}
}

u_int16_t vin_phi_revision(struct vinetic_context *ctx)
{
	u_int16_t rev;

	if (ioctl(ctx->dev_fd, VINETIC_REVISION, &rev) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_REVISION) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		rev = 0;
	}
	ctx->revision = rev;

	return rev;
}

u_int16_t vin_phi_checksum(struct vinetic_context *ctx)
{
	u_int16_t csum;

	if (ioctl(ctx->dev_fd, VINETIC_CHECKSUM, &csum) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_CHECKSUM) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		csum = 0;
	}

	return csum;
}

int vin_phi_disable_interrupt(struct vinetic_context *ctx)
{
	int res;

	if ((res = ioctl(ctx->dev_fd, VINETIC_DISABLE_IRQ, NULL)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s ioctl(ctx->dev_fd, VINETIC_DISABLE_IRQ) failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
	}

	return res;
}

int vin_check_mbx_empty(struct vinetic_context *ctx)
{
	ssize_t res;
	size_t cnt;
	union vin_cmd cmd;
	struct vin_read_bxsr bxsr;

	res = 0;
	cnt = 255;
	cmd.full = 0;
	cmd.parts.first.full = VIN_rBXSR;

	for (;;) {
		if ((res = vin_read(ctx, cmd, &bxsr, sizeof(struct vin_read_bxsr))) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_check_mbx_empty_end;
		}
		if (bxsr.bxsr2.bits.mbx_empty) {
			break;
		}
		if (!cnt--) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s wait for mbx_empty time is out", __LINE__, __PRETTY_FUNCTION__);
			res = -EIO;
			goto vin_check_mbx_empty_end;
		}
		usleep(1000);
	}

vin_check_mbx_empty_end:
	return res;
}

int vin_wait_dl_rdy(struct vinetic_context *ctx)
{
	ssize_t res;
	size_t cnt;
	union vin_cmd cmd;
	struct vin_read_hwsr hwsr;

	res = 0;
	cmd.full = 0;
	cmd.parts.first.full = VIN_rHWSR;
	cnt = 8000;
	for (;;) {
		if ((res = vin_read(ctx, cmd, &hwsr, sizeof(struct vin_read_hwsr))) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_wait_dl_ready_end;
		}
		if (hwsr.hwsr2.bits.dl_rdy) {
			break;
		}
		if (!cnt--) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s wait for dl_rdy time is out", __LINE__, __PRETTY_FUNCTION__);
			goto vin_wait_dl_ready_end;
		}
		usleep(125);
	}

vin_wait_dl_ready_end:
	return res;
}

int vin_download_edsp_firmware(struct vinetic_context *ctx)
{
	union vin_cmd cmd;
	union vin_cmd_short cmd_short;

	struct vin_cmd_eop_set_pram_address cmd_eop_set_pram_address;
	struct vin_cmd_eop_access_pram cmd_eop_access_pram;

	struct vin_cmd_eop_set_dram_address cmd_eop_set_dram_address;
	struct vin_cmd_eop_access_dram cmd_eop_access_dram;
#if 0
	struct vin_cmd_eop_crc_pram cmd_eop_crc_pram;
	struct vin_cmd_eop_crc_dram cmd_eop_crc_dram;
	u_int16_t seg_addr_start_high;
	u_int16_t seg_addr_start_low;
	u_int32_t seg_addr_start;
	u_int16_t seg_addr_end_high;
	u_int16_t seg_addr_end_low;
	u_int32_t seg_addr_end;
#endif
	ssize_t res;
	size_t i;
	size_t seg_size;
	size_t seg_chunk_size;

	int fd = -1;
	off_t fd_offset;
	struct stat fd_stat;

	struct vin_xram_segment_header {
		u_int32_t size;
		u_int16_t high;
		u_int16_t low;
	} __attribute__((packed)) xram_segment_header;

	// Check for MBX-EMPTY
	if (vin_check_mbx_empty(ctx) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	// Maximazing Command In-Box
	cmd_short.full = VIN_wMAXCBX;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	// Load EDSP Micro Program
	cmd_short.full = VIN_wLEMP;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	// open PRAM file
	if ((fd = open(ctx->pram_path, O_RDONLY)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s open() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	if (fstat(fd, &fd_stat) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fstat() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	fd_offset = 0;
	while (fd_offset < fd_stat.st_size) {
		// Read PRAM segment header
		if (lseek(fd, fd_offset, SEEK_SET) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_edsp_firmware_error;
		}
		if (read(fd, &xram_segment_header, sizeof(struct vin_xram_segment_header)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_edsp_firmware_error;
		}
		fd_offset += sizeof(struct vin_xram_segment_header);
#if 0
		seg_size = be32toh(xram_segment_header.size);
#else
		seg_size = ntohl(xram_segment_header.size);
#endif
		// Set PRAM Address
		cmd_eop_set_pram_address.header.parts.first.bits.rw = VIN_WRITE;
		cmd_eop_set_pram_address.header.parts.first.bits.sc = VIN_SC_NO;
		cmd_eop_set_pram_address.header.parts.first.bits.bc = VIN_BC_NO;
		cmd_eop_set_pram_address.header.parts.first.bits.cmd = VIN_CMD_EOP;
		cmd_eop_set_pram_address.header.parts.first.bits.res = 0;
		cmd_eop_set_pram_address.header.parts.first.bits.chan = 0;
		cmd_eop_set_pram_address.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
		cmd_eop_set_pram_address.header.parts.second.eop.bits.ecmd  = VIN_EOP_SETPRAM;
		cmd_eop_set_pram_address.header.parts.second.eop.bits.length = 2;
#if 0
		cmd_eop_set_pram_address.high_addres1 = be16toh(xram_segment_header.high);
		cmd_eop_set_pram_address.low_addres1 = be16toh(xram_segment_header.low);
#else
		cmd_eop_set_pram_address.high_addres1 = ntohs(xram_segment_header.high);
		cmd_eop_set_pram_address.low_addres1 = ntohs(xram_segment_header.low);
#endif
		if ((res = vin_write(ctx, 0, &cmd_eop_set_pram_address, sizeof(union vin_cmd) + sizeof(u_int16_t) * 2)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_edsp_firmware_error;
		}
		while (seg_size) {
			// Check for MBX-EMPTY
			if (vin_check_mbx_empty(ctx) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_edsp_firmware_error;
			}
			// Read PRAM segment data
			if (lseek(fd, fd_offset, SEEK_SET) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_edsp_firmware_error;
			}
			seg_chunk_size = (seg_size < 252) ? (seg_size) : (252);
			if (read(fd, cmd_eop_access_pram.data, seg_chunk_size * 2) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_edsp_firmware_error;
			}
			fd_offset += seg_chunk_size * 2;
			for (i = 0; i < seg_chunk_size; i++) {
#if 0
				cmd_eop_access_pram.data[i] = be16toh(cmd_eop_access_pram.data[i]);
#else
				cmd_eop_access_pram.data[i] = ntohs(cmd_eop_access_pram.data[i]);
#endif
			}
			// Access PRAM
			cmd_eop_access_pram.header.parts.first.bits.rw = VIN_WRITE;
			cmd_eop_access_pram.header.parts.first.bits.sc = VIN_SC_NO;
			cmd_eop_access_pram.header.parts.first.bits.bc = VIN_BC_NO;
			cmd_eop_access_pram.header.parts.first.bits.cmd = VIN_CMD_EOP;
			cmd_eop_access_pram.header.parts.first.bits.res = 0;
			cmd_eop_access_pram.header.parts.first.bits.chan = 0;
			cmd_eop_access_pram.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
			cmd_eop_access_pram.header.parts.second.eop.bits.ecmd  = VIN_EOP_ACCESSPRAM;
			cmd_eop_access_pram.header.parts.second.eop.bits.length = seg_chunk_size;
			if ((res = vin_write(ctx, 0, &cmd_eop_access_pram, sizeof(union vin_cmd) + seg_chunk_size * 2)) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_edsp_firmware_error;
			}
			seg_size -= seg_chunk_size;
		}
	}
#if 0
	// Reset PRAM CRC
	cmd_eop_crc_pram.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_crc_pram.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_crc_pram.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_crc_pram.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_crc_pram.header.parts.first.bits.res = 0;
	cmd_eop_crc_pram.header.parts.first.bits.chan = 0;
	cmd_eop_crc_pram.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
	cmd_eop_crc_pram.header.parts.second.eop.bits.ecmd  = VIN_EOP_CRC_PRAM;
	cmd_eop_crc_pram.header.parts.second.eop.bits.length = 0;
	if ((res = vin_write(ctx, &cmd_eop_crc_pram, sizeof(union vin_cmd)) < 0)) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	fd_offset = 0;
	while (fd_offset < fd_stat.st_size)
	{
		// Read PRAM segment header
		if (lseek(fd, fd_offset, SEEK_SET) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
		if (read(fd, &xram_segment_header, sizeof(struct vin_xram_segment_header)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
		seg_addr_start_high = be16toh(xram_segment_header.high);
		seg_addr_start_low = be16toh(xram_segment_header.low);
		seg_addr_start = (seg_addr_start_high << 16) + (seg_addr_start_low << 0);
		seg_size = be32toh(xram_segment_header.size);
		seg_addr_end = seg_addr_start + (seg_size/3)*2 - 2;
		seg_addr_end_high = (seg_addr_end >> 16) & 0xffff;
		seg_addr_end_low = (seg_addr_end >> 0) & 0xffff;
// 		PRINTF("pram seg %08x:%08x - %lu\n", seg_addr_start, seg_addr_end, (unsigned long int)seg_size);
		fd_offset += sizeof(struct vin_xram_segment_header) + seg_size*2;
		// Set PRAM Address
		cmd_eop_set_pram_address.header.parts.first.bits.rw = VIN_WRITE;
		cmd_eop_set_pram_address.header.parts.first.bits.sc = VIN_SC_NO;
		cmd_eop_set_pram_address.header.parts.first.bits.bc = VIN_BC_NO;
		cmd_eop_set_pram_address.header.parts.first.bits.cmd = VIN_CMD_EOP;
		cmd_eop_set_pram_address.header.parts.first.bits.res = 0;
		cmd_eop_set_pram_address.header.parts.first.bits.chan = 0;
		cmd_eop_set_pram_address.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
		cmd_eop_set_pram_address.header.parts.second.eop.bits.ecmd  = VIN_EOP_SETPRAM;
		cmd_eop_set_pram_address.header.parts.second.eop.bits.length = 4;
		cmd_eop_set_pram_address.high_addres1 = seg_addr_start_high;
		cmd_eop_set_pram_address.low_addres1 = seg_addr_start_low;
		cmd_eop_set_pram_address.high_addres2 = seg_addr_end_high;
		cmd_eop_set_pram_address.low_addres2 = seg_addr_end_low;
		if ((res = vin_write(ctx, &cmd_eop_set_pram_address, sizeof(union vin_cmd) + sizeof(u_int16_t)*4)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
		// Read PRAM CRC
		cmd_eop_crc_pram.header.parts.first.bits.rw = VIN_READ;
		cmd_eop_crc_pram.header.parts.first.bits.sc = VIN_SC_NO;
		cmd_eop_crc_pram.header.parts.first.bits.bc = VIN_BC_NO;
		cmd_eop_crc_pram.header.parts.first.bits.cmd = VIN_CMD_EOP;
		cmd_eop_crc_pram.header.parts.first.bits.res = 0;
		cmd_eop_crc_pram.header.parts.first.bits.chan = 0;
		cmd_eop_crc_pram.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
		cmd_eop_crc_pram.header.parts.second.eop.bits.ecmd  = VIN_EOP_CRC_PRAM;
		cmd_eop_crc_pram.header.parts.second.eop.bits.length = 1;
		if ((res = vin_read(ctx, cmd_eop_crc_pram.header, &cmd_eop_crc_pram, sizeof(struct vin_cmd_eop_crc_pram)) < 0)) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
	}
// 	PRINTF("\npram: checksum=0x%04x\n", cmd_eop_crc_pram.crc);
#endif
	close(fd);

	// Check for MBX-EMPTY
	if (vin_check_mbx_empty(ctx) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}

	// open DRAM file
	if ((fd = open(ctx->dram_path, O_RDONLY)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s open() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	if (fstat(fd, &fd_stat) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fstat() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	fd_offset = 0;
	while (fd_offset < fd_stat.st_size) {
		// Read DRAM segment header
		if (lseek(fd, fd_offset, SEEK_SET) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_edsp_firmware_error;
		}
		if (read(fd, &xram_segment_header, sizeof(struct vin_xram_segment_header)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_edsp_firmware_error;
		}
		fd_offset += sizeof(struct vin_xram_segment_header);
		// Set DRAM Address
		cmd_eop_set_dram_address.header.parts.first.bits.rw = VIN_WRITE;
		cmd_eop_set_dram_address.header.parts.first.bits.sc = VIN_SC_NO;
		cmd_eop_set_dram_address.header.parts.first.bits.bc = VIN_BC_NO;
		cmd_eop_set_dram_address.header.parts.first.bits.cmd = VIN_CMD_EOP;
		cmd_eop_set_dram_address.header.parts.first.bits.res = 0;
		cmd_eop_set_dram_address.header.parts.first.bits.chan = 0;
		cmd_eop_set_dram_address.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
		cmd_eop_set_dram_address.header.parts.second.eop.bits.ecmd  = VIN_EOP_SET_DRAM;
		cmd_eop_set_dram_address.header.parts.second.eop.bits.length = 1;
#if 0
		cmd_eop_set_dram_address.addres1 = be16toh(xram_segment_header.low);
#else
		cmd_eop_set_dram_address.addres1 = ntohs(xram_segment_header.low);
#endif
		if ((res = vin_write(ctx, 0, &cmd_eop_set_dram_address, sizeof(union vin_cmd) + sizeof(u_int16_t))) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_edsp_firmware_error;
		}
#if 0
		seg_size = be32toh(xram_segment_header.size);
#else
		seg_size = ntohl(xram_segment_header.size);
#endif
		while (seg_size) {
			// Check for MBX-EMPTY
			if (vin_check_mbx_empty(ctx) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_edsp_firmware_error;
			}
			// Read DRAM segment data
			if (lseek(fd, fd_offset, SEEK_SET) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s lseek() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_edsp_firmware_error;
			}
			seg_chunk_size = (seg_size < 252) ? (seg_size) : (252);
			if (read(fd, cmd_eop_access_dram.data, seg_chunk_size*2) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_edsp_firmware_error;
			}
			fd_offset += seg_chunk_size * 2;
			for (i = 0; i < seg_chunk_size; i++) {
#if 0
				cmd_eop_access_dram.data[i] = be16toh(cmd_eop_access_dram.data[i]);
#else
				cmd_eop_access_dram.data[i] = ntohs(cmd_eop_access_dram.data[i]);
#endif
			}
			// Access DRAM
			cmd_eop_access_dram.header.parts.first.bits.rw = VIN_WRITE;
			cmd_eop_access_dram.header.parts.first.bits.sc = VIN_SC_NO;
			cmd_eop_access_dram.header.parts.first.bits.bc = VIN_BC_NO;
			cmd_eop_access_dram.header.parts.first.bits.cmd = VIN_CMD_EOP;
			cmd_eop_access_dram.header.parts.first.bits.res = 0;
			cmd_eop_access_dram.header.parts.first.bits.chan = 0;
			cmd_eop_access_dram.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
			cmd_eop_access_dram.header.parts.second.eop.bits.ecmd  = VIN_EOP_ACCESS_DRAM;
			cmd_eop_access_dram.header.parts.second.eop.bits.length = seg_chunk_size;
			if ((res = vin_write(ctx, 0, &cmd_eop_access_dram, sizeof(union vin_cmd) + seg_chunk_size * 2)) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_edsp_firmware_error;
			}
			seg_size -= seg_chunk_size;
		}
	}
#if 0
	// Reset DRAM CRC
	cmd_eop_crc_dram.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_crc_dram.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_crc_dram.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_crc_dram.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_crc_dram.header.parts.first.bits.res = 0;
	cmd_eop_crc_dram.header.parts.first.bits.chan = 0;
	cmd_eop_crc_dram.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
	cmd_eop_crc_dram.header.parts.second.eop.bits.ecmd  = VIN_EOP_CRC_DRAM;
	cmd_eop_crc_dram.header.parts.second.eop.bits.length = 0;
	if ((res = vin_write(ctx, &cmd_eop_crc_dram, sizeof(union vin_cmd)) < 0)) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	fd_offset = 0;
	while (fd_offset < fd_stat.st_size)
	{
		// Read DRAM segment header
		if (lseek(fd, fd_offset, SEEK_SET) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
		if (read(fd, &xram_segment_header, sizeof(struct vin_xram_segment_header)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
		seg_addr_start_high = be16toh(xram_segment_header.high);
		seg_addr_start_low = be16toh(xram_segment_header.low);
		seg_addr_start = (seg_addr_start_high << 16) + (seg_addr_start_low << 0);
		seg_size = be32toh(xram_segment_header.size);
		seg_addr_end = seg_addr_start + seg_size - 1;
		seg_addr_end_high = (seg_addr_end >> 16) & 0xffff;
		seg_addr_end_low = (seg_addr_end >> 0) & 0xffff;
// 		PRINTF("dram seg %08x:%08x - %lu\n", seg_addr_start, seg_addr_end, (unsigned long int)seg_size);
		fd_offset += sizeof(struct vin_xram_segment_header) + seg_size*2;
		// Set DRAM Address
		cmd_eop_set_dram_address.header.parts.first.bits.rw = VIN_WRITE;
		cmd_eop_set_dram_address.header.parts.first.bits.sc = VIN_SC_NO;
		cmd_eop_set_dram_address.header.parts.first.bits.bc = VIN_BC_NO;
		cmd_eop_set_dram_address.header.parts.first.bits.cmd = VIN_CMD_EOP;
		cmd_eop_set_dram_address.header.parts.first.bits.res = 0;
		cmd_eop_set_dram_address.header.parts.first.bits.chan = 0;
		cmd_eop_set_dram_address.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
		cmd_eop_set_dram_address.header.parts.second.eop.bits.ecmd  = VIN_EOP_SET_DRAM;
		cmd_eop_set_dram_address.header.parts.second.eop.bits.length = 2;
		cmd_eop_set_dram_address.addres1 = seg_addr_start_low;
		cmd_eop_set_dram_address.addres1 = seg_addr_end_low;
		if ((res = vin_write(ctx, &cmd_eop_set_dram_address, sizeof(union vin_cmd) + sizeof(u_int16_t)*2)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
		// Read DRAM CRC
		cmd_eop_crc_dram.header.parts.first.bits.rw = VIN_READ;
		cmd_eop_crc_dram.header.parts.first.bits.sc = VIN_SC_NO;
		cmd_eop_crc_dram.header.parts.first.bits.bc = VIN_BC_NO;
		cmd_eop_crc_dram.header.parts.first.bits.cmd = VIN_CMD_EOP;
		cmd_eop_crc_dram.header.parts.first.bits.res = 0;
		cmd_eop_crc_dram.header.parts.first.bits.chan = 0;
		cmd_eop_crc_dram.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
		cmd_eop_crc_dram.header.parts.second.eop.bits.ecmd  = VIN_EOP_CRC_DRAM;
		cmd_eop_crc_dram.header.parts.second.eop.bits.length = 1;
		if ((res = vin_read(ctx, cmd_eop_crc_dram.header, &cmd_eop_crc_dram, sizeof(struct vin_cmd_eop_crc_dram)) < 0)) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
	}
// 	PRINTF("\ndram: checksum=0x%04x\n", cmd_eop_crc_dram.crc);
#endif
	close(fd);

	// Check for MBX-EMPTY
	if (vin_check_mbx_empty(ctx) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	// Download End
	cmd.parts.first.bits.rw = VIN_WRITE;
	cmd.parts.first.bits.sc = VIN_SC_NO;
	cmd.parts.first.bits.bc = VIN_BC_NO;
	cmd.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd.parts.first.bits.res = 0;
	cmd.parts.first.bits.chan = 0;
	cmd.parts.second.eop.bits.mod = VIN_MOD_TEST;
	cmd.parts.second.eop.bits.ecmd  = VIN_EOP_DOWNLOAD_END;
	cmd.parts.second.eop.bits.length = 0;
	if ((res = vin_write(ctx, 0, &cmd, sizeof(union vin_cmd))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	// Wait for DL-RDY
	if (vin_wait_dl_rdy(ctx) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_wait_dl_rdy() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	// Check for MBX-EMPTY
	if (vin_check_mbx_empty(ctx) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}
	// Minimazing Command In-Box
	cmd_short.full = VIN_wMINCBX;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}

	// Start EDSP 
	cmd_short.full = VIN_wSTEDSP;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_edsp_firmware_error;
	}

	return 0;

vin_download_edsp_firmware_error:
	if (fd > 0) close(fd);
	return -1;
}

int vin_download_alm_dsp(struct vinetic_context *ctx, char *path)
{
	struct vin_cmd_eop_set_fpi_address cmd_eop_set_fpi_address;
	struct vin_cmd_eop_access_fpi_memory cmd_eop_access_fpi_memory;
// 	struct vin_cmd_eop_crc_fpi cmd_eop_crc_fpi;

	u_int32_t addr_start, addr_end;
	size_t data_size, data_chunk_size;
	u_int16_t *data = NULL;
	u_int16_t *datap;
// 	u_int16_t checksum;
// 	u_int16_t dschkr;

	FILE * fp = NULL;
	char fpbuf[32];
	int flag_address = 0;
	int flag_data = 0;
	int flag_checksum = 0;
	int flag_dschkr = 0;

	ssize_t res;
	size_t i;
	u_int32_t tmp_u32;

	// open ALM DSP patch file
	if (!(fp = fopen(path, "r"))) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fopen() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_alm_dsp_error;
	}

	// get address
	while (fgets(fpbuf, sizeof(fpbuf), fp))
	{
		if (!strncasecmp(fpbuf, "[ADDRESS]", strlen("[ADDRESS]"))) {
			flag_address = 1;
			break;
		}
	}
	if (flag_address) {
		// get start address
		if (fgets(fpbuf, sizeof(fpbuf), fp)) {
			if (sscanf(fpbuf, "0x%08X", &tmp_u32) != 1) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s sscanf() can't get start address", __LINE__, __PRETTY_FUNCTION__);
				goto vin_download_alm_dsp_error;
			}
		} else {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fgets() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_alm_dsp_error;
		}
		addr_start = tmp_u32;
		// get end address
		if (fgets(fpbuf, sizeof(fpbuf), fp)) {
			if (sscanf(fpbuf, "0x%08X", &tmp_u32) != 1) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s sscanf() can't get end address", __LINE__, __PRETTY_FUNCTION__);
				goto vin_download_alm_dsp_error;
			}
		} else {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fgets() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_alm_dsp_error;
		}
		addr_end = tmp_u32;
	} else {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s can't get ADDRESS section", __LINE__, __PRETTY_FUNCTION__);
		goto vin_download_alm_dsp_error;
	}

	// get data
	data_size = (addr_end - addr_start + 1);

	while (fgets(fpbuf, sizeof(fpbuf), fp)) {
		if (!strncasecmp(fpbuf, "[DATA]", strlen("[DATA]"))) {
			flag_data = 1;
			break;
		}
	}
	if (flag_data) {
		// alloc data buffer
		if ((data = malloc(data_size * 2))) {
			datap = data;
			for (i = 0; i < data_size; i++) {
				if (fgets(fpbuf, sizeof(fpbuf), fp)) {
					if (sscanf(fpbuf, "0x%04X", &tmp_u32) != 1) {
						vin_message_stack_printf(ctx, "libvinetic.c:%d in %s sscanf() can't get DATA", __LINE__, __PRETTY_FUNCTION__);
						goto vin_download_alm_dsp_error;
					}
				} else {
					vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fgets() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
					goto vin_download_alm_dsp_error;
				}
				*datap++ = tmp_u32 & 0xffff;
			}
		} else {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s malloc() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_alm_dsp_error;
		}
	}

	// get checksum
	while (fgets(fpbuf, sizeof(fpbuf), fp)) {
		if (!strncasecmp(fpbuf, "[CHECKSUM]", strlen("[CHECKSUM]"))) {
			flag_checksum = 1;
			break;
		}
	}
	if (flag_checksum) {
		if (fgets(fpbuf, sizeof(fpbuf), fp)) {
			if (sscanf(fpbuf, "0x%04X", &tmp_u32) != 1) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s sscanf() can't get CHECKSUM", __LINE__, __PRETTY_FUNCTION__);
				goto vin_download_alm_dsp_error;
			}
		} else {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fgets() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_alm_dsp_error;
		}
// 		checksum = tmp_u32 & 0xffff;
	}

	// get dschkr
	while (fgets(fpbuf, sizeof(fpbuf), fp)) {
		if (!strncasecmp(fpbuf, "[DSCHKR]", strlen("[DSCHKR]"))) {
			flag_dschkr = 1;
			break;
		}
	}
	if (flag_dschkr) {
		if (fgets(fpbuf, sizeof(fpbuf), fp)) {
			if (sscanf(fpbuf, "0x%04X", &tmp_u32) != 1) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s sscanf() can't get DSCHKR", __LINE__, __PRETTY_FUNCTION__);
				goto vin_download_alm_dsp_error;
			}
		} else {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fgets() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_alm_dsp_error;
		}
// 		dschkr = tmp_u32 & 0xffff;
	}

	// Check for MBX-EMPTY
	if (vin_check_mbx_empty(ctx) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_alm_dsp_error;
	}
	// Set FPI Address
	cmd_eop_set_fpi_address.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_set_fpi_address.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_set_fpi_address.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_set_fpi_address.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_set_fpi_address.header.parts.first.bits.res = 0;
	cmd_eop_set_fpi_address.header.parts.first.bits.chan = 0;
	cmd_eop_set_fpi_address.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
	cmd_eop_set_fpi_address.header.parts.second.eop.bits.ecmd  = VIN_EOP_SET_FPI;
	cmd_eop_set_fpi_address.header.parts.second.eop.bits.length = 4;
	cmd_eop_set_fpi_address.high_addres1 = (addr_start >> 16) & 0xffff;
	cmd_eop_set_fpi_address.low_addres1 = addr_start & 0xffff;
	cmd_eop_set_fpi_address.high_addres2 = (addr_end >> 16) & 0xffff;
	cmd_eop_set_fpi_address.low_addres2 = addr_end & 0xffff;
	if ((res = vin_write(ctx, 0, &cmd_eop_set_fpi_address, sizeof(struct vin_cmd_eop_set_fpi_address))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_alm_dsp_error;
	}
#if 0
	// CRC FPI - reset
	cmd_eop_crc_fpi.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_crc_fpi.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_crc_fpi.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_crc_fpi.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_crc_fpi.header.parts.first.bits.res = 0;
	cmd_eop_crc_fpi.header.parts.first.bits.chan = 0;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.ecmd  = VIN_EOP_CRC_FPI;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.length = 0;
	if ((res = vin_write(ctx, &cmd_eop_crc_fpi, sizeof(union vin_cmd))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_alm_dsp_error;
	}
#endif
	// Write FPI data
	datap = data;
	while (data_size) {
		// Check for MBX-EMPTY
		if (vin_check_mbx_empty(ctx) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_alm_dsp_error;
		}
		data_chunk_size = (data_size < 29) ? (data_size) : (29);
		for (i = 0; i < data_chunk_size; i++) {
			memcpy(&cmd_eop_access_fpi_memory.data[i], datap++, sizeof(u_int16_t));
		}
		// Access FPI Memory
		cmd_eop_access_fpi_memory.header.parts.first.bits.rw = VIN_WRITE;
		cmd_eop_access_fpi_memory.header.parts.first.bits.sc = VIN_SC_NO;
		cmd_eop_access_fpi_memory.header.parts.first.bits.bc = VIN_BC_NO;
		cmd_eop_access_fpi_memory.header.parts.first.bits.cmd = VIN_CMD_EOP;
		cmd_eop_access_fpi_memory.header.parts.first.bits.res = 0;
		cmd_eop_access_fpi_memory.header.parts.first.bits.chan = 0;
		cmd_eop_access_fpi_memory.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
		cmd_eop_access_fpi_memory.header.parts.second.eop.bits.ecmd  = VIN_EOP_ACCESS_FPI;
		cmd_eop_access_fpi_memory.header.parts.second.eop.bits.length = data_chunk_size;
		if ((res = vin_write(ctx, 0, &cmd_eop_access_fpi_memory, sizeof(union vin_cmd) + data_chunk_size*2)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_download_alm_dsp_error;
		}
		data_size -= data_chunk_size;
	}

#if 0
	// CRC FPI - reset
	cmd_eop_crc_fpi.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_crc_fpi.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_crc_fpi.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_crc_fpi.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_crc_fpi.header.parts.first.bits.res = 0;
	cmd_eop_crc_fpi.header.parts.first.bits.chan = 0;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.ecmd  = VIN_EOP_CRC_FPI;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.length = 0;
	if ((res = vin_write(ctx, &cmd_eop_crc_fpi, sizeof(union vin_cmd))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_alm_dsp_error;
	}
	// CRC FPI - read
	cmd_eop_crc_fpi.header.parts.first.bits.rw = VIN_READ;
	cmd_eop_crc_fpi.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_crc_fpi.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_crc_fpi.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_crc_fpi.header.parts.first.bits.res = 0;
	cmd_eop_crc_fpi.header.parts.first.bits.chan = 0;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.ecmd  = VIN_EOP_CRC_FPI;
	cmd_eop_crc_fpi.header.parts.second.eop.bits.length = 1;
	if (vin_read(ctx, cmd_eop_crc_fpi.header, &cmd_eop_crc_fpi, sizeof(struct vin_cmd_eop_crc_fpi)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_alm_dsp_error;
	}
// 	PRINTF("crc=0x%04x\n", cmd_eop_crc_fpi.crc);
#endif
#if 0
	// DSCHKR
	struct vin_cmd_sop_dschkr cmd_sop_dschkr;
	// read
	cmd_sop_dschkr.header.parts.first.bits.rw = VIN_READ;
	cmd_sop_dschkr.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_sop_dschkr.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_sop_dschkr.header.parts.first.bits.cmd = VIN_CMD_SOP;
	cmd_sop_dschkr.header.parts.second.sop.bits.offset = VIN_SOP_DSCHKR;
	cmd_sop_dschkr.header.parts.second.sop.bits.length  = 1;
	if (vin_read(ctx, cmd_sop_dschkr.header, &cmd_sop_dschkr, sizeof(struct vin_cmd_sop_dschkr)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_alm_dsp_error;
	}
// 	PRINTF("crc=0x%04x\n", cmd_sop_dschkr.ds_check);
#endif

	free(data);
	fclose(fp);
	return 0;

vin_download_alm_dsp_error:
	if (data) free(data);
	if (fp) fclose(fp);
	return -1;
}

int vin_jump_alm_dsp(struct vinetic_context *ctx, unsigned int chan)
{
	struct vin_cmd_sop_ccr cmd_sop_ccr;

	ssize_t res;

	// Common Configuration Register
	cmd_sop_ccr.header.parts.first.bits.rw = VIN_WRITE;
	cmd_sop_ccr.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_sop_ccr.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_sop_ccr.header.parts.first.bits.cmd = VIN_CMD_SOP;
	cmd_sop_ccr.header.parts.first.bits.res = 0;
	cmd_sop_ccr.header.parts.first.bits.chan = chan;
	cmd_sop_ccr.header.parts.second.sop.bits.offset = VIN_SOP_CCR;
	cmd_sop_ccr.header.parts.second.sop.bits.length = 1;
	cmd_sop_ccr.sop_ccr.pd_cbias = 0;
	cmd_sop_ccr.sop_ccr.pd_cvcm = 0;
	cmd_sop_ccr.sop_ccr.jump_ac1 = 0;
	cmd_sop_ccr.sop_ccr.jump_ac2 = 0;
	cmd_sop_ccr.sop_ccr.jump_ac3 = 1;
	cmd_sop_ccr.sop_ccr.jump_dc = 0;
	cmd_sop_ccr.sop_ccr.res0 = 0;
	if ((res = vin_write(ctx, 0, &cmd_sop_ccr, sizeof(struct vin_cmd_sop_ccr))) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_jump_alm_dsp_error;
	}
	return 0;

vin_jump_alm_dsp_error:
	return -1;
}

int vin_download_cram(struct vinetic_context *ctx, unsigned int chan, char *path)
{

	FILE * fp = NULL;
	char fpbuf[512];
	char hdr[16];
	u_int32_t cfd[6];

	int res;

	struct vin_cmd_cop_generic cmd_cop_generic;

	// open CRAM *.byt file
	if (!(fp = fopen(path, "r"))) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s fopen() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_download_cram_error;
	}

	while (fgets(fpbuf, sizeof(fpbuf), fp)) {
		res = sscanf(fpbuf, " %[A-Z0-9_] = 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X", hdr, &cfd[0], &cfd[1], &cfd[2], &cfd[3], &cfd[4], &cfd[5]);
		if (res == 7) {
			// Check for MBX-EMPTY
			if (vin_check_mbx_empty(ctx) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_check_mbx_empty() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_cram_error;
			}
			// write coefficient
			cmd_cop_generic.header.parts.first.full = (cfd[0] & 0xffff) + chan;
			cmd_cop_generic.header.parts.second.full = cfd[1] & 0xffff;
			cmd_cop_generic.word[0] = cfd[2] & 0xffff;
			cmd_cop_generic.word[1] = cfd[3] & 0xffff;
			cmd_cop_generic.word[2] = cfd[4] & 0xffff;
			cmd_cop_generic.word[3] = cfd[5] & 0xffff;
			if ((res = vin_write(ctx, 0, &cmd_cop_generic, sizeof(struct vin_cmd_cop_generic))) < 0) {
				vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
				goto vin_download_cram_error;
			}
		}
	}
	fclose(fp);
	return 0;

vin_download_cram_error:
	if (fp) fclose(fp);
	return -1;
}

int vin_write_sop_generic(struct vinetic_context *ctx, unsigned int chan, u_int16_t offset, u_int16_t data)
{
	struct vin_cmd_sop_generic cmd_sop_generic;

	// Write SOP generic Register
	cmd_sop_generic.header.parts.first.bits.rw = VIN_WRITE;
	cmd_sop_generic.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_sop_generic.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_sop_generic.header.parts.first.bits.cmd = VIN_CMD_SOP;
	cmd_sop_generic.header.parts.first.bits.res = 0;
	cmd_sop_generic.header.parts.first.bits.chan = chan;
	cmd_sop_generic.header.parts.second.sop.bits.offset = offset;
	cmd_sop_generic.header.parts.second.sop.bits.length = 1;
	cmd_sop_generic.word = data;
	if (vin_write(ctx, 1, &cmd_sop_generic, sizeof(struct vin_cmd_sop_generic)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_write_sop_generic_error;
	}

	return 0;

vin_write_sop_generic_error:
	return -1;
}

int vin_set_opmode(struct vinetic_context *ctx, unsigned int ch, unsigned int mode)
{
	union vin_cmd_short cmd;

	ctx->ali_opmode[ch] = mode;

	cmd.bits.rw = VIN_WRITE;
	cmd.bits.sc = 1;
	cmd.bits.bc = 0;
	cmd.bits.om = 1;
	cmd.bits.cmd = (mode >> 4) & 0xf;
	cmd.bits.subcmd = mode & 0xf;
	cmd.bits.chan = ch;

	if (vin_write(ctx, 0, &cmd, sizeof(union vin_cmd_short)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_set_opmode_error;
	}
	return 0;

vin_set_opmode_error:
	return -1;
}

int vin_pcm_interface_control(struct vinetic_context *ctx, unsigned int rw)
{
	struct vin_cmd_eop_pcm_interface_control cmd_eop_pcm_interface_control;

	cmd_eop_pcm_interface_control.header.parts.first.bits.rw = rw;
	cmd_eop_pcm_interface_control.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_pcm_interface_control.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_pcm_interface_control.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_pcm_interface_control.header.parts.first.bits.res = 0;
	cmd_eop_pcm_interface_control.header.parts.first.bits.chan = 0;
	cmd_eop_pcm_interface_control.header.parts.second.eop.bits.mod = VIN_MOD_PCM;
	cmd_eop_pcm_interface_control.header.parts.second.eop.bits.ecmd  = VIN_EOP_PCM_CONT;
	cmd_eop_pcm_interface_control.header.parts.second.eop.bits.length = sizeof(struct vin_eop_pcm_interface_control) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_pcm_interface_control.eop_pcm_interface_control, &ctx->eop_pcm_interface_control, sizeof(struct vin_eop_pcm_interface_control));
		if (vin_write(ctx, 1, &cmd_eop_pcm_interface_control, sizeof(struct vin_cmd_eop_pcm_interface_control)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_pcm_interface_control_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_pcm_interface_control.header, &cmd_eop_pcm_interface_control, sizeof(struct vin_cmd_eop_pcm_interface_control)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_pcm_interface_control_error;
		}
		memcpy(&ctx->eop_pcm_interface_control, &cmd_eop_pcm_interface_control.eop_pcm_interface_control, sizeof(struct vin_eop_pcm_interface_control));
	}

	return 0;

vin_pcm_interface_control_error:
	return -1;
}

int vin_pcm_interface_channel(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_pcm_interface_channel cmd_eop_pcm_interface_channel;

	cmd_eop_pcm_interface_channel.header.parts.first.bits.rw = rw;
	cmd_eop_pcm_interface_channel.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_pcm_interface_channel.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_pcm_interface_channel.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_pcm_interface_channel.header.parts.first.bits.res = 0;
	cmd_eop_pcm_interface_channel.header.parts.first.bits.chan = ch;
	cmd_eop_pcm_interface_channel.header.parts.second.eop.bits.mod = VIN_MOD_PCM;
	cmd_eop_pcm_interface_channel.header.parts.second.eop.bits.ecmd  = VIN_EOP_PCM_CHAN;
	cmd_eop_pcm_interface_channel.header.parts.second.eop.bits.length = sizeof(struct vin_eop_pcm_interface_channel) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_pcm_interface_channel.eop_pcm_interface_channel, &ctx->eop_pcm_interface_channel[ch], sizeof(struct vin_eop_pcm_interface_channel));
		if (vin_write(ctx, 1, &cmd_eop_pcm_interface_channel, sizeof(struct vin_cmd_eop_pcm_interface_channel)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_pcm_interface_channel_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_pcm_interface_channel.header, &cmd_eop_pcm_interface_channel, sizeof(struct vin_cmd_eop_pcm_interface_channel)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_pcm_interface_channel_error;
		}
		memcpy(&ctx->eop_pcm_interface_channel[ch], &cmd_eop_pcm_interface_channel.eop_pcm_interface_channel, sizeof(struct vin_eop_pcm_interface_channel));
	}

	return 0;

vin_pcm_interface_channel_error:
	return -1;
}

int vin_pcm_near_end_lec(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_pcm_near_end_lec cmd_eop_pcm_near_end_lec;

	cmd_eop_pcm_near_end_lec.header.parts.first.bits.rw = rw;
	cmd_eop_pcm_near_end_lec.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_pcm_near_end_lec.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_pcm_near_end_lec.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_pcm_near_end_lec.header.parts.first.bits.res = 0;
	cmd_eop_pcm_near_end_lec.header.parts.first.bits.chan = ch;
	cmd_eop_pcm_near_end_lec.header.parts.second.eop.bits.mod = VIN_MOD_PCM;
	cmd_eop_pcm_near_end_lec.header.parts.second.eop.bits.ecmd  = VIN_EOP_NEAR_END_LEC;
	cmd_eop_pcm_near_end_lec.header.parts.second.eop.bits.length = sizeof(struct vin_eop_pcm_near_end_lec) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_pcm_near_end_lec.eop_pcm_near_end_lec, &ctx->eop_pcm_near_end_lec[ch], sizeof(struct vin_eop_pcm_near_end_lec));
		if (vin_write(ctx, 1, &cmd_eop_pcm_near_end_lec, sizeof(struct vin_cmd_eop_pcm_near_end_lec)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_pcm_near_end_lec_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_pcm_near_end_lec.header, &cmd_eop_pcm_near_end_lec, sizeof(struct vin_cmd_eop_pcm_near_end_lec)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_pcm_near_end_lec_error;
		}
		memcpy(&ctx->eop_pcm_near_end_lec[ch], &cmd_eop_pcm_near_end_lec.eop_pcm_near_end_lec, sizeof(struct vin_eop_pcm_near_end_lec));
	}

	return 0;

vin_pcm_near_end_lec_error:
	return -1;
}

int vin_ali_control(struct vinetic_context *ctx, unsigned int rw)
{
	struct vin_cmd_eop_ali_control cmd_eop_ali_control;

	cmd_eop_ali_control.header.parts.first.bits.rw = rw;
	cmd_eop_ali_control.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_ali_control.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_ali_control.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_ali_control.header.parts.first.bits.res = 0;
	cmd_eop_ali_control.header.parts.first.bits.chan = 0;
	cmd_eop_ali_control.header.parts.second.eop.bits.mod = VIN_MOD_ALI;
	cmd_eop_ali_control.header.parts.second.eop.bits.ecmd  = VIN_EOP_ALI_CONT;
	cmd_eop_ali_control.header.parts.second.eop.bits.length = sizeof(struct vin_eop_ali_control) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_ali_control.eop_ali_control, &ctx->eop_ali_control, sizeof(struct vin_eop_ali_control));
		if (vin_write(ctx, 1, &cmd_eop_ali_control, sizeof(struct vin_cmd_eop_ali_control)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_ali_control_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_ali_control.header, &cmd_eop_ali_control, sizeof(struct vin_cmd_eop_ali_control)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_ali_control_error;
		}
		memcpy(&ctx->eop_ali_control, &cmd_eop_ali_control.eop_ali_control, sizeof(struct vin_eop_ali_control));
	}

	return 0;

vin_ali_control_error:
	return -1;
}

int vin_ali_channel(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_ali_channel cmd_eop_ali_channel;

	cmd_eop_ali_channel.header.parts.first.bits.rw = rw;
	cmd_eop_ali_channel.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_ali_channel.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_ali_channel.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_ali_channel.header.parts.first.bits.res = 0;
	cmd_eop_ali_channel.header.parts.first.bits.chan = ch;
	cmd_eop_ali_channel.header.parts.second.eop.bits.mod = VIN_MOD_ALI;
	cmd_eop_ali_channel.header.parts.second.eop.bits.ecmd  = VIN_EOP_ALI_CHAN;
	cmd_eop_ali_channel.header.parts.second.eop.bits.length = sizeof(struct vin_eop_ali_channel) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_ali_channel.eop_ali_channel, &ctx->eop_ali_channel[ch], sizeof(struct vin_eop_ali_channel));
		if (vin_write(ctx, 1, &cmd_eop_ali_channel, sizeof(struct vin_cmd_eop_ali_channel)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_ali_channel_error;
		}
		return 0;
	} else {
		if (vin_read(ctx, cmd_eop_ali_channel.header, &cmd_eop_ali_channel, sizeof(struct vin_cmd_eop_ali_channel)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_ali_channel_error;
		}
		memcpy(&ctx->eop_ali_channel[ch], &cmd_eop_ali_channel.eop_ali_channel, sizeof(struct vin_eop_ali_channel));
	}

vin_ali_channel_error:
	return -1;
}

int vin_ali_near_end_lec(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_ali_near_end_lec cmd_eop_ali_near_end_lec;

	cmd_eop_ali_near_end_lec.header.parts.first.bits.rw = rw;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.res = 0;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.chan = ch;
	cmd_eop_ali_near_end_lec.header.parts.second.eop.bits.mod = VIN_MOD_ALI;
	cmd_eop_ali_near_end_lec.header.parts.second.eop.bits.ecmd  = VIN_EOP_ALI_NEAR_END_LEC;
	cmd_eop_ali_near_end_lec.header.parts.second.eop.bits.length = sizeof(struct vin_eop_ali_near_end_lec) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_ali_near_end_lec.eop_ali_near_end_lec, &ctx->eop_ali_near_end_lec[ch], sizeof(struct vin_eop_ali_near_end_lec));
		if (vin_write(ctx, 1, &cmd_eop_ali_near_end_lec, sizeof(struct vin_cmd_eop_ali_near_end_lec)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_ali_near_end_lec_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_ali_near_end_lec.header, &cmd_eop_ali_near_end_lec, sizeof(struct vin_cmd_eop_ali_near_end_lec)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_ali_near_end_lec_error;
		}
		memcpy(&ctx->eop_ali_near_end_lec[ch], &cmd_eop_ali_near_end_lec.eop_ali_near_end_lec, sizeof(struct vin_eop_ali_near_end_lec));
	}

	return 0;

vin_ali_near_end_lec_error:
	return -1;
}

int vin_signaling_control(struct vinetic_context *ctx)
{
	struct vin_cmd_eop_signaling_control cmd_eop_signaling_control;

	cmd_eop_signaling_control.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_signaling_control.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_signaling_control.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_signaling_control.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_signaling_control.header.parts.first.bits.res = 0;
	cmd_eop_signaling_control.header.parts.first.bits.chan = 0;
	cmd_eop_signaling_control.header.parts.second.eop.bits.mod = VIN_MOD_SIG;
	cmd_eop_signaling_control.header.parts.second.eop.bits.ecmd  = VIN_EOP_SIG_CONT;
	cmd_eop_signaling_control.header.parts.second.eop.bits.length = sizeof(struct vin_eop_signaling_control) / 2;
	memcpy(&cmd_eop_signaling_control.eop_signaling_control, &ctx->eop_signaling_control, sizeof(struct vin_eop_signaling_control));
	if (vin_write(ctx, 1, &cmd_eop_signaling_control, sizeof(struct vin_cmd_eop_signaling_control)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_signaling_control_error;
	}
	return 0;

vin_signaling_control_error:
	return -1;
}

int vin_signaling_channel(struct vinetic_context *ctx, unsigned int ch)
{
	struct vin_cmd_eop_signaling_channel cmd_eop_signaling_channel;

	cmd_eop_signaling_channel.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_signaling_channel.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_signaling_channel.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_signaling_channel.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_signaling_channel.header.parts.first.bits.res = 0;
	cmd_eop_signaling_channel.header.parts.first.bits.chan = ch;
	cmd_eop_signaling_channel.header.parts.second.eop.bits.mod = VIN_MOD_SIG;
	cmd_eop_signaling_channel.header.parts.second.eop.bits.ecmd  = VIN_EOP_SIG_CHAN;
	cmd_eop_signaling_channel.header.parts.second.eop.bits.length = sizeof(struct vin_eop_signaling_channel) / 2;
	memcpy(&cmd_eop_signaling_channel.eop_signaling_channel, &ctx->eop_signaling_channel[ch], sizeof(struct vin_eop_signaling_channel));
	if (vin_write(ctx, 1, &cmd_eop_signaling_channel, sizeof(struct vin_cmd_eop_signaling_channel)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_signaling_channel_error;
	}
	return 0;

vin_signaling_channel_error:
	return -1;
}

int vin_dtmfat_generator(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_dtmfat_generator cmd_eop_dtmfat_generator;

	cmd_eop_dtmfat_generator.header.parts.first.bits.rw = rw;
	cmd_eop_dtmfat_generator.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_dtmfat_generator.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_dtmfat_generator.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_dtmfat_generator.header.parts.first.bits.res = 0;
	cmd_eop_dtmfat_generator.header.parts.first.bits.chan = ch;
	cmd_eop_dtmfat_generator.header.parts.second.eop.bits.mod = VIN_MOD_SIG;
	cmd_eop_dtmfat_generator.header.parts.second.eop.bits.ecmd  = VIN_EOP_DTMFATGEN;
	cmd_eop_dtmfat_generator.header.parts.second.eop.bits.length = sizeof(struct vin_eop_dtmfat_generator) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_dtmfat_generator.eop_dtmfat_generator, &ctx->eop_dtmfat_generator[ch], sizeof(struct vin_eop_dtmfat_generator));
		if (vin_write(ctx, 1, &cmd_eop_dtmfat_generator, sizeof(struct vin_cmd_eop_dtmfat_generator)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_dtmfat_generator_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_dtmfat_generator.header, &cmd_eop_dtmfat_generator, sizeof(struct vin_cmd_eop_dtmfat_generator)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_dtmfat_generator_error;
		}
		memcpy(&ctx->eop_dtmfat_generator[ch], &cmd_eop_dtmfat_generator.eop_dtmfat_generator, sizeof(struct vin_eop_dtmfat_generator));
	}

	return 0;

vin_dtmfat_generator_error:
	return -1;
}

int vin_dtmf_receiver(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_dtmf_receiver cmd_eop_dtmf_receiver;

	cmd_eop_dtmf_receiver.header.parts.first.bits.rw = rw;
	cmd_eop_dtmf_receiver.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_dtmf_receiver.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_dtmf_receiver.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_dtmf_receiver.header.parts.first.bits.res = 0;
	cmd_eop_dtmf_receiver.header.parts.first.bits.chan = ch;
	cmd_eop_dtmf_receiver.header.parts.second.eop.bits.mod = VIN_MOD_SIG;
	cmd_eop_dtmf_receiver.header.parts.second.eop.bits.ecmd  = VIN_EOP_DTMFREC;
	cmd_eop_dtmf_receiver.header.parts.second.eop.bits.length = sizeof(struct vin_eop_dtmf_receiver) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_dtmf_receiver.eop_dtmf_receiver, &ctx->eop_dtmf_receiver[ch], sizeof(struct vin_eop_dtmf_receiver));
		if (vin_write(ctx, 1, &cmd_eop_dtmf_receiver, sizeof(struct vin_cmd_eop_dtmf_receiver)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_dtmf_receiver_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_dtmf_receiver.header, &cmd_eop_dtmf_receiver, sizeof(struct vin_cmd_eop_dtmf_receiver)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_dtmf_receiver_error;
		}
		memcpy(&ctx->eop_dtmf_receiver[ch], &cmd_eop_dtmf_receiver.eop_dtmf_receiver, sizeof(struct vin_eop_dtmf_receiver));
	}

	return 0;

vin_dtmf_receiver_error:
	return -1;
}

int vin_utg(struct vinetic_context *ctx, unsigned int ch)
{
	struct vin_cmd_eop_utg cmd_eop_utg;

	cmd_eop_utg.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_utg.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_utg.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_utg.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_utg.header.parts.first.bits.res = 0;
	cmd_eop_utg.header.parts.first.bits.chan = ch;
	cmd_eop_utg.header.parts.second.eop.bits.mod = VIN_MOD_SIG;
	cmd_eop_utg.header.parts.second.eop.bits.ecmd  = VIN_EOP_UTG;
	cmd_eop_utg.header.parts.second.eop.bits.length = sizeof(struct vin_eop_utg) / 2;
	memcpy(&cmd_eop_utg.eop_utg, &ctx->eop_utg[ch], sizeof(struct vin_eop_utg));
	if (vin_write(ctx, 1, &cmd_eop_utg, sizeof(struct vin_cmd_eop_utg)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_utg_error;
	}
	return 0;

vin_utg_error:
	return -1;
}

int vin_signaling_channel_configuration_rtp_support(struct vinetic_context *ctx, unsigned int ch)
{
	struct vin_cmd_eop_signaling_channel_configuration_rtp_support cmd_eop_signaling_channel_configuration_rtp_support;

	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.first.bits.res = 0;
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.first.bits.chan = ch;
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.second.eop.bits.mod = VIN_MOD_SIG;
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.second.eop.bits.ecmd  = VIN_EOP_SIG_CONF_RTP;
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.second.eop.bits.length = sizeof(struct vin_eop_signaling_channel_configuration_rtp_support) / 2;
	memcpy(&cmd_eop_signaling_channel_configuration_rtp_support.eop_signaling_channel_configuration_rtp_support,
			&ctx->eop_signaling_channel_configuration_rtp_support[ch],
			sizeof(struct vin_eop_signaling_channel_configuration_rtp_support));
	if (vin_write(ctx, 1, &cmd_eop_signaling_channel_configuration_rtp_support, sizeof(struct vin_cmd_eop_signaling_channel_configuration_rtp_support)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_signaling_channel_configuration_rtp_support_error;
	}
	return 0;

vin_signaling_channel_configuration_rtp_support_error:
	return -1;
}

int vin_coder_control(struct vinetic_context *ctx)
{
	struct vin_cmd_eop_coder_control cmd_eop_coder_control;

	cmd_eop_coder_control.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_coder_control.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_coder_control.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_coder_control.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_coder_control.header.parts.first.bits.res = 0;
	cmd_eop_coder_control.header.parts.first.bits.chan = 0;
	cmd_eop_coder_control.header.parts.second.eop.bits.mod = VIN_MOD_CODER;
	cmd_eop_coder_control.header.parts.second.eop.bits.ecmd  = VIN_EOP_CODER_CONT;
	cmd_eop_coder_control.header.parts.second.eop.bits.length = sizeof(struct vin_eop_coder_control) / 2;
	memcpy(&cmd_eop_coder_control.eop_coder_control, &ctx->eop_coder_control, sizeof(struct vin_eop_coder_control));
	if (vin_write(ctx, 1, &cmd_eop_coder_control, sizeof(struct vin_cmd_eop_coder_control)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_coder_control_error;
	}
	return 0;

vin_coder_control_error:
	return -1;
}

int vin_coder_channel_speech_compression(struct vinetic_context *ctx, unsigned int ch)
{
	struct vin_cmd_eop_coder_channel_speech_compression cmd_eop_coder_channel_speech_compression;

	cmd_eop_coder_channel_speech_compression.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_coder_channel_speech_compression.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_coder_channel_speech_compression.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_coder_channel_speech_compression.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_coder_channel_speech_compression.header.parts.first.bits.res = 0;
	cmd_eop_coder_channel_speech_compression.header.parts.first.bits.chan = ch;
	cmd_eop_coder_channel_speech_compression.header.parts.second.eop.bits.mod = VIN_MOD_CODER;
	cmd_eop_coder_channel_speech_compression.header.parts.second.eop.bits.ecmd  = VIN_EOP_CODER_CHAN_SC;
	cmd_eop_coder_channel_speech_compression.header.parts.second.eop.bits.length = sizeof(struct vin_eop_coder_channel_speech_compression) / 2;
	memcpy(&cmd_eop_coder_channel_speech_compression.eop_coder_channel_speech_compression,
			&ctx->eop_coder_channel_speech_compression[ch],
			sizeof(struct vin_eop_coder_channel_speech_compression));
	if (vin_write(ctx, 1, &cmd_eop_coder_channel_speech_compression, sizeof(struct vin_cmd_eop_coder_channel_speech_compression)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_coder_channel_speech_compression_error;
	}
	return 0;

vin_coder_channel_speech_compression_error:
	return -1;
}

int vin_coder_configuration_rtp_support(struct vinetic_context *ctx)
{
	struct vin_cmd_eop_coder_configuration_rtp_support cmd_eop_coder_configuration_rtp_support;

	cmd_eop_coder_configuration_rtp_support.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_coder_configuration_rtp_support.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_coder_configuration_rtp_support.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_coder_configuration_rtp_support.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_coder_configuration_rtp_support.header.parts.first.bits.res = 0;
	cmd_eop_coder_configuration_rtp_support.header.parts.first.bits.chan = 0;
	cmd_eop_coder_configuration_rtp_support.header.parts.second.eop.bits.mod = VIN_MOD_CODER;
	cmd_eop_coder_configuration_rtp_support.header.parts.second.eop.bits.ecmd  = VIN_EOP_CODER_CONF;
	cmd_eop_coder_configuration_rtp_support.header.parts.second.eop.bits.length = sizeof(struct vin_eop_coder_configuration_rtp_support) / 2;
	memcpy(&cmd_eop_coder_configuration_rtp_support.eop_coder_configuration_rtp_support,
			&ctx->eop_coder_configuration_rtp_support,
			sizeof(struct vin_eop_coder_configuration_rtp_support));
	if (vin_write(ctx, 1, &cmd_eop_coder_configuration_rtp_support, sizeof(struct vin_cmd_eop_coder_configuration_rtp_support)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_coder_configuration_rtp_support_error;
	}
	return 0;

vin_coder_configuration_rtp_support_error:
	return -1;
}

int vin_coder_channel_configuration_rtp_support(struct vinetic_context *ctx, unsigned int ch)
{
	struct vin_cmd_eop_coder_channel_configuration_rtp_support cmd_eop_coder_channel_configuration_rtp_support;

	cmd_eop_coder_channel_configuration_rtp_support.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.first.bits.res = 0;
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.first.bits.chan = ch;
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.second.eop.bits.mod = VIN_MOD_CODER;
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.second.eop.bits.ecmd  = VIN_EOP_CODER_CONF_RTP;
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.second.eop.bits.length = sizeof(struct vin_eop_coder_channel_configuration_rtp_support) / 2;
	memcpy(&cmd_eop_coder_channel_configuration_rtp_support.eop_coder_channel_configuration_rtp_support,
			&ctx->eop_coder_channel_configuration_rtp_support[ch],
			sizeof(struct vin_eop_coder_channel_configuration_rtp_support));
	if (vin_write(ctx, 1, &cmd_eop_coder_channel_configuration_rtp_support, sizeof(struct vin_cmd_eop_coder_channel_configuration_rtp_support)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_coder_channel_configuration_rtp_support_error;
	}
	return 0;

vin_coder_channel_configuration_rtp_support_error:
	return -1;
}

int vin_coder_channel_jb_statistic_reset(struct vinetic_context *ctx, unsigned int chan)
{
	union vin_cmd cmd;

	cmd.parts.first.bits.rw = VIN_WRITE;
	cmd.parts.first.bits.sc = VIN_SC_NO;
	cmd.parts.first.bits.bc = VIN_BC_NO;
	cmd.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd.parts.first.bits.res = 0;
	cmd.parts.first.bits.chan = chan & 0x7;
	cmd.parts.second.eop.bits.mod = VIN_MOD_CODER;
	cmd.parts.second.eop.bits.ecmd  = VIN_EOP_CODER_JBSTAT;
	cmd.parts.second.eop.bits.length = 0;
	if (vin_write(ctx, 1, &cmd, sizeof(union vin_cmd)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_coder_channel_jb_statistic_reset_error;
	}
	return 0;

vin_coder_channel_jb_statistic_reset_error:
	return -1;
}

int vin_dtmfat_generator_coefficients(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_dtmfat_generator_coefficients cmd_eop_dtmfat_generator_coefficients;

	cmd_eop_dtmfat_generator_coefficients.header.parts.first.bits.rw = rw;
	cmd_eop_dtmfat_generator_coefficients.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_dtmfat_generator_coefficients.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_dtmfat_generator_coefficients.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_dtmfat_generator_coefficients.header.parts.first.bits.res = 0;
	cmd_eop_dtmfat_generator_coefficients.header.parts.first.bits.chan = ch;
	cmd_eop_dtmfat_generator_coefficients.header.parts.second.eop.bits.mod = VIN_MOD_RESOURCE;
	cmd_eop_dtmfat_generator_coefficients.header.parts.second.eop.bits.ecmd  = VIN_EOP_DTMFATCOEFF;
	cmd_eop_dtmfat_generator_coefficients.header.parts.second.eop.bits.length = sizeof(struct vin_eop_dtmfat_generator_coefficients) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_dtmfat_generator_coefficients.eop_dtmfat_generator_coefficients, &ctx->eop_dtmfat_generator_coefficients[ch], sizeof(struct vin_eop_dtmfat_generator_coefficients));
		if (vin_write(ctx, 1, &cmd_eop_dtmfat_generator_coefficients, sizeof(struct vin_cmd_eop_dtmfat_generator_coefficients)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_dtmfat_generator_coefficients_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_dtmfat_generator_coefficients.header, &cmd_eop_dtmfat_generator_coefficients, sizeof(struct vin_cmd_eop_dtmfat_generator_coefficients)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_dtmfat_generator_coefficients_error;
		}
		memcpy(&ctx->eop_dtmfat_generator_coefficients[ch], &cmd_eop_dtmfat_generator_coefficients.eop_dtmfat_generator_coefficients, sizeof(struct vin_eop_dtmfat_generator_coefficients));
	}

	return 0;

vin_dtmfat_generator_coefficients_error:
	return -1;
}

int vin_dtmfat_generator_data(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_dtmfat_generator_data cmd_eop_dtmfat_generator_data;

	cmd_eop_dtmfat_generator_data.header.parts.first.bits.rw = rw;
	cmd_eop_dtmfat_generator_data.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_dtmfat_generator_data.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_dtmfat_generator_data.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_dtmfat_generator_data.header.parts.first.bits.res = 0;
	cmd_eop_dtmfat_generator_data.header.parts.first.bits.chan = ch;
	cmd_eop_dtmfat_generator_data.header.parts.second.eop.bits.mod = VIN_MOD_RESOURCE;
	cmd_eop_dtmfat_generator_data.header.parts.second.eop.bits.ecmd  = VIN_EOP_DTM_AT_GEN_DATA;
	cmd_eop_dtmfat_generator_data.header.parts.second.eop.bits.length = sizeof(struct vin_eop_dtmfat_generator_data) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_dtmfat_generator_data.eop_dtmfat_generator_data, &ctx->eop_dtmfat_generator_data[ch], sizeof(struct vin_eop_dtmfat_generator_data));
		if (vin_write(ctx, 1, &cmd_eop_dtmfat_generator_data, sizeof(struct vin_cmd_eop_dtmfat_generator_data)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_dtmfat_generator_data_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_dtmfat_generator_data.header, &cmd_eop_dtmfat_generator_data, sizeof(struct vin_cmd_eop_dtmfat_generator_data)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_dtmfat_generator_data_error;
		}
		memcpy(&ctx->eop_dtmfat_generator_data[ch], &cmd_eop_dtmfat_generator_data.eop_dtmfat_generator_data, sizeof(struct vin_eop_dtmfat_generator_data));
	}

	return 0;

vin_dtmfat_generator_data_error:
	return -1;
}

int vin_utg_coefficients(struct vinetic_context *ctx, unsigned int rw, unsigned int ch)
{
	struct vin_cmd_eop_utg_coefficients cmd_eop_utg_coefficients;

	cmd_eop_utg_coefficients.header.parts.first.bits.rw = rw;
	cmd_eop_utg_coefficients.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_utg_coefficients.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_utg_coefficients.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_utg_coefficients.header.parts.first.bits.res = 0;
	cmd_eop_utg_coefficients.header.parts.first.bits.chan = ch;
	cmd_eop_utg_coefficients.header.parts.second.eop.bits.mod = VIN_MOD_RESOURCE;
	cmd_eop_utg_coefficients.header.parts.second.eop.bits.ecmd  = VIN_EOP_UTG_COEFF;
	cmd_eop_utg_coefficients.header.parts.second.eop.bits.length = sizeof(struct vin_eop_utg_coefficients) / 2;

	if (rw == VIN_WRITE) {
		memcpy(&cmd_eop_utg_coefficients.eop_utg_coefficients, &ctx->eop_utg_coefficients[ch], sizeof(struct vin_eop_utg_coefficients));
		if (vin_write(ctx, 1, &cmd_eop_utg_coefficients, sizeof(struct vin_cmd_eop_utg_coefficients)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_utg_coefficients_error;
		}
	} else {
		if (vin_read(ctx, cmd_eop_utg_coefficients.header, &cmd_eop_utg_coefficients, sizeof(struct vin_cmd_eop_utg_coefficients)) < 0) {
			vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_read() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
			goto vin_utg_coefficients_error;
		}
		memcpy(&ctx->eop_utg_coefficients[ch], &cmd_eop_utg_coefficients.eop_utg_coefficients, sizeof(struct vin_eop_utg_coefficients));
	}

	return 0;

vin_utg_coefficients_error:
	return -1;
}

int vin_utg_set_asterisk_tone(struct vinetic_context *ctx, unsigned int ch, const char *tone)
{
#define MAX_TONE_CHUNK_LENGTH 32
#define MAX_TONE_MASK_COUNT 6
#define MAX_FREQUENCIES_COUNT 4
	struct mask {
		char data[MAX_TONE_CHUNK_LENGTH];
		int frequency1;
		int frequency2;
		int modulation;
		int duration;
		int stop;
		size_t repetition;
	} masks[MAX_TONE_MASK_COUNT];
	u_int32_t frequencies[4];

	size_t count;
	size_t i, j;
	char *part;
	char *input;
	char *next;

	char buf[256];
	size_t len;

	u_int32_t freq1, freq2;

	count = 0;
	memset(masks, 0 , sizeof(masks));
	for (i = 0; i < MAX_TONE_MASK_COUNT; i++) {
		masks[i].frequency1 = -1;
		masks[i].frequency2 = -1;
	}
	memset(frequencies, 0 , sizeof(frequencies));
	// get tone copy
	next = input = strdup(tone);
	if (!input) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s strdup() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_utg_set_asterisk_tone_error;
	}
	// parse tone
	while (next) {
		// check for repetition
		len = 0;
		for (i = 0; i < count; i++) {
			if (i) {
				len += snprintf(buf+len, sizeof(buf)-len, ",%s", masks[i].data);
			} else {
				len += snprintf(buf+len, sizeof(buf)-len, "%s", masks[i].data);
			}
		}
		if (len && (len <= strlen(next)) && !strncmp(next, buf, len) && ((next[len] == '\0') || (next[len] == ','))) {
			masks[count-1].repetition++;
			next += len;
			if (*next == ',') next++;
			continue;
		}
		// get next tone chunk
		part = strsep(&next, ",");
		if (!strlen(part) || !strcmp(part, "0")) break;
		// get empty mask
		for (i = 0; i < MAX_TONE_MASK_COUNT; i++) {
			if (!strlen(masks[i].data)) {
				break;
			}
		}
		if (i == MAX_TONE_MASK_COUNT) {
			break;
		}
		snprintf(masks[i].data, MAX_TONE_CHUNK_LENGTH, "%s", part);
		count++;
		// parse tone chunk
		freq1 = 0;
		freq2 = 0;
		if (*part == '!') {
			masks[i].stop = 1;
			part++;
		}
		if (sscanf(part, "%u+%u/%d", &freq1,  &freq2, &masks[i].duration) == 3) {
			masks[i].modulation = 0;
		} else if (sscanf(part, "%u*%u/%d", &freq1,  &freq2, &masks[i].duration) == 3) {
			masks[i].modulation = 1;
		} else if (sscanf(part, "%u+%u", &freq1,  &freq2) == 2) {
			masks[i].modulation = 0;
			masks[i].duration = -1;
		} else if (sscanf(part, "%u*%u", &freq1,  &freq2) == 2) {
			masks[i].modulation = 1;
			masks[i].duration = -1;
		} else if (sscanf(part, "%u/%d", &freq1, &masks[i].duration) == 2) {
			masks[i].modulation = 0;
		} else if (sscanf(part, "%u", &freq1) == 1) {
			masks[i].modulation = 0;
			masks[i].duration = -1;
		} else {
			break;
		}
		if (freq1 > 0) {
			for (j = 0; j < MAX_FREQUENCIES_COUNT; j++) {
				if (frequencies[j] == freq1) {
					masks[i].frequency1 = j;
					break;
				}
			}
			if (j == MAX_FREQUENCIES_COUNT) {
				for (j = 0; j < MAX_FREQUENCIES_COUNT; j++) {
					if (!frequencies[j]) {
						break;
					}
				}
				if (j == MAX_FREQUENCIES_COUNT) {
					break;
				}
				frequencies[j] = freq1;
				masks[i].frequency1 = j;
			}
		}
		if (freq2 > 0) {
			for (j = 0; j < MAX_FREQUENCIES_COUNT; j++) {
				if (frequencies[j] == freq2) {
					masks[i].frequency2 = j;
					break;
				}
			}
			if (j == MAX_FREQUENCIES_COUNT) {
				for (j = 0; j < MAX_FREQUENCIES_COUNT; j++) {
					if (!frequencies[j]) {
						break;
					}
				}
				if (j == MAX_FREQUENCIES_COUNT) {
					break;
				}
				frequencies[j] = freq2;
				masks[i].frequency2 = j;
			}
		}
	}
	// set UTG coefficients
	vin_utg_coefficients_set_fd_in_att(ctx, ch, 0.f);
	vin_utg_coefficients_set_fd_in_sp(ctx, ch, 0.f);
	vin_utg_coefficients_set_fd_in_tim(ctx, ch, 0);
	vin_utg_coefficients_set_fd_ot_sp(ctx, ch, 0.f);
	vin_utg_coefficients_set_fd_ot_tim(ctx, ch, 0);
	vin_utg_coefficients_set_mod_12(ctx, ch, 0.9f);
	vin_utg_coefficients_set_f1(ctx, ch, frequencies[0]);
	vin_utg_coefficients_set_f2(ctx, ch, frequencies[1]);
	vin_utg_coefficients_set_f3(ctx, ch, frequencies[2]);
	vin_utg_coefficients_set_f4(ctx, ch, frequencies[3]);
	vin_utg_coefficients_set_lev_1(ctx, ch, 0.f);
	vin_utg_coefficients_set_lev_2(ctx, ch, 0.f);
	vin_utg_coefficients_set_lev_3(ctx, ch, 0.f);
	vin_utg_coefficients_set_lev_4(ctx, ch, 0.f);
	vin_utg_coefficients_set_go_add_a(ctx, ch, 0.f);
	vin_utg_coefficients_set_go_add_b(ctx, ch, 0.f);
	// mask 1
	vin_utg_coefficients_set_t_1(ctx, ch, masks[0].duration);
	vin_utg_coefficients_set_msk_1_nxt(ctx, ch, (((masks[0].repetition) || (count == 1))?0:1));
	vin_utg_coefficients_set_msk_1_fi(ctx, ch, 0);
	vin_utg_coefficients_set_msk_1_fo(ctx, ch, 0);
	vin_utg_coefficients_set_msk_1_f1(ctx, ch, (((masks[0].frequency1 == 0) || (masks[0].frequency2 == 0))?1:0));
	vin_utg_coefficients_set_msk_1_f2(ctx, ch, (((masks[0].frequency1 == 1) || (masks[0].frequency2 == 1))?1:0));
	vin_utg_coefficients_set_msk_1_f3(ctx, ch, (((masks[0].frequency1 == 2) || (masks[0].frequency2 == 2))?1:0));
	vin_utg_coefficients_set_msk_1_f4(ctx, ch, (((masks[0].frequency1 == 3) || (masks[0].frequency2 == 3))?1:0));
	vin_utg_coefficients_set_msk_1_m12(ctx, ch, masks[0].modulation);
	vin_utg_coefficients_set_msk_1_rep(ctx, ch, ((masks[0].repetition > 7)?7:masks[0].repetition));
	vin_utg_coefficients_set_msk_1_sa(ctx, ch, ((count == 1)?masks[0].stop:0));
	// mask 2
	vin_utg_coefficients_set_t_2(ctx, ch, masks[1].duration);
	vin_utg_coefficients_set_msk_2_nxt(ctx, ch, (((masks[1].repetition) || (count == 2))?0:2));
	vin_utg_coefficients_set_msk_2_fi(ctx, ch, 0);
	vin_utg_coefficients_set_msk_2_fo(ctx, ch, 0);
	vin_utg_coefficients_set_msk_2_f1(ctx, ch, (((masks[1].frequency1 == 0) || (masks[1].frequency2 == 0))?1:0));
	vin_utg_coefficients_set_msk_2_f2(ctx, ch, (((masks[1].frequency1 == 1) || (masks[1].frequency2 == 1))?1:0));
	vin_utg_coefficients_set_msk_2_f3(ctx, ch, (((masks[1].frequency1 == 2) || (masks[1].frequency2 == 2))?1:0));
	vin_utg_coefficients_set_msk_2_f4(ctx, ch, (((masks[1].frequency1 == 3) || (masks[1].frequency2 == 3))?1:0));
	vin_utg_coefficients_set_msk_2_m12(ctx, ch, masks[1].modulation);
	vin_utg_coefficients_set_msk_2_rep(ctx, ch, ((masks[1].repetition > 7)?7:masks[1].repetition));
	vin_utg_coefficients_set_msk_2_sa(ctx, ch, ((count == 2)?masks[1].stop:0));
	// mask 3
	vin_utg_coefficients_set_t_3(ctx, ch, masks[2].duration);
	vin_utg_coefficients_set_msk_3_nxt(ctx, ch, (((masks[2].repetition) || (count == 3))?0:3));
	vin_utg_coefficients_set_msk_3_fi(ctx, ch, 0);
	vin_utg_coefficients_set_msk_3_fo(ctx, ch, 0);
	vin_utg_coefficients_set_msk_3_f1(ctx, ch, (((masks[2].frequency1 == 0) || (masks[2].frequency2 == 0))?1:0));
	vin_utg_coefficients_set_msk_3_f2(ctx, ch, (((masks[2].frequency1 == 1) || (masks[2].frequency2 == 1))?1:0));
	vin_utg_coefficients_set_msk_3_f3(ctx, ch, (((masks[2].frequency1 == 2) || (masks[2].frequency2 == 2))?1:0));
	vin_utg_coefficients_set_msk_3_f4(ctx, ch, (((masks[2].frequency1 == 3) || (masks[2].frequency2 == 3))?1:0));
	vin_utg_coefficients_set_msk_3_m12(ctx, ch, masks[2].modulation);
	vin_utg_coefficients_set_msk_3_rep(ctx, ch, ((masks[2].repetition > 7)?7:masks[2].repetition));
	vin_utg_coefficients_set_msk_3_sa(ctx, ch, ((count == 3)?masks[2].stop:0));
	// mask 4
	vin_utg_coefficients_set_t_4(ctx, ch, masks[3].duration);
	vin_utg_coefficients_set_msk_4_nxt(ctx, ch, (((masks[3].repetition) || (count == 4))?0:4));
	vin_utg_coefficients_set_msk_4_fi(ctx, ch, 0);
	vin_utg_coefficients_set_msk_4_fo(ctx, ch, 0);
	vin_utg_coefficients_set_msk_4_f1(ctx, ch, (((masks[3].frequency1 == 0) || (masks[3].frequency2 == 0))?1:0));
	vin_utg_coefficients_set_msk_4_f2(ctx, ch, (((masks[3].frequency1 == 1) || (masks[3].frequency2 == 1))?1:0));
	vin_utg_coefficients_set_msk_4_f3(ctx, ch, (((masks[3].frequency1 == 2) || (masks[3].frequency2 == 2))?1:0));
	vin_utg_coefficients_set_msk_4_f4(ctx, ch, (((masks[3].frequency1 == 3) || (masks[3].frequency2 == 3))?1:0));
	vin_utg_coefficients_set_msk_4_m12(ctx, ch, masks[3].modulation);
	vin_utg_coefficients_set_msk_4_rep(ctx, ch, ((masks[3].repetition > 7)?7:masks[3].repetition));
	vin_utg_coefficients_set_msk_4_sa(ctx, ch, ((count == 4)?masks[3].stop:0));
	// mask 5
	vin_utg_coefficients_set_t_5(ctx, ch, masks[4].duration);
	vin_utg_coefficients_set_msk_5_nxt(ctx, ch, (((masks[4].repetition) || (count == 5))?0:5));
	vin_utg_coefficients_set_msk_5_fi(ctx, ch, 0);
	vin_utg_coefficients_set_msk_5_fo(ctx, ch, 0);
	vin_utg_coefficients_set_msk_5_f1(ctx, ch, (((masks[4].frequency1 == 0) || (masks[4].frequency2 == 0))?1:0));
	vin_utg_coefficients_set_msk_5_f2(ctx, ch, (((masks[4].frequency1 == 1) || (masks[4].frequency2 == 1))?1:0));
	vin_utg_coefficients_set_msk_5_f3(ctx, ch, (((masks[4].frequency1 == 2) || (masks[4].frequency2 == 2))?1:0));
	vin_utg_coefficients_set_msk_5_f4(ctx, ch, (((masks[4].frequency1 == 3) || (masks[4].frequency2 == 3))?1:0));
	vin_utg_coefficients_set_msk_5_m12(ctx, ch, masks[4].modulation);
	vin_utg_coefficients_set_msk_5_rep(ctx, ch, ((masks[4].repetition > 7)?7:masks[4].repetition));
	vin_utg_coefficients_set_msk_5_sa(ctx, ch, ((count == 5)?masks[4].stop:0));
	// mask 6
	vin_utg_coefficients_set_t_6(ctx, ch, masks[5].duration);
	vin_utg_coefficients_set_msk_6_nxt(ctx, ch, (((masks[5].repetition) || (count == 6))?0:0));
	vin_utg_coefficients_set_msk_6_fi(ctx, ch, 0);
	vin_utg_coefficients_set_msk_6_fo(ctx, ch, 0);
	vin_utg_coefficients_set_msk_6_f1(ctx, ch, (((masks[5].frequency1 == 0) || (masks[5].frequency2 == 0))?1:0));
	vin_utg_coefficients_set_msk_6_f2(ctx, ch, (((masks[5].frequency1 == 1) || (masks[5].frequency2 == 1))?1:0));
	vin_utg_coefficients_set_msk_6_f3(ctx, ch, (((masks[5].frequency1 == 2) || (masks[5].frequency2 == 2))?1:0));
	vin_utg_coefficients_set_msk_6_f4(ctx, ch, (((masks[5].frequency1 == 3) || (masks[5].frequency2 == 3))?1:0));
	vin_utg_coefficients_set_msk_6_m12(ctx, ch, masks[5].modulation);
	vin_utg_coefficients_set_msk_6_rep(ctx, ch, ((masks[5].repetition > 7)?7:masks[5].repetition));
	vin_utg_coefficients_set_msk_6_sa(ctx, ch, ((count == 6)?masks[5].stop:0));

	free(input);
	return 0;

vin_utg_set_asterisk_tone_error:
	if (input) free(input);
	return -1;
}

int vin_set_endian_mode(struct vinetic_context *ctx, int mode)
{
	struct vin_cmd_eop_endian_control cmd_eop_endian_control;
	// Write Endian Control
	cmd_eop_endian_control.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_endian_control.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_endian_control.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_endian_control.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_endian_control.header.parts.first.bits.res = 0;
	cmd_eop_endian_control.header.parts.first.bits.chan = 0;
	cmd_eop_endian_control.header.parts.second.eop.bits.mod = VIN_MOD_CONT;
	cmd_eop_endian_control.header.parts.second.eop.bits.ecmd  = VIN_EOP_ENDIAN_CONT;
	cmd_eop_endian_control.header.parts.second.eop.bits.length = 1;
	cmd_eop_endian_control.eop_endian_control.res = 0;
	cmd_eop_endian_control.eop_endian_control.le = mode;
	if (vin_write(ctx, 1, &cmd_eop_endian_control, sizeof(struct vin_cmd_eop_endian_control)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_set_endian_mode_error;
	}
	return 0;

vin_set_endian_mode_error:
	return -1;
}

int vin_read_fw_version(struct vinetic_context *ctx)
{
	struct vin_cmd_eop_edsp_sw_version_register cmd_eop_edsp_sw_version_register;

	// Read EDSP SW Version Register
	cmd_eop_edsp_sw_version_register.header.parts.first.bits.rw = VIN_READ;
	cmd_eop_edsp_sw_version_register.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_edsp_sw_version_register.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_edsp_sw_version_register.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_edsp_sw_version_register.header.parts.first.bits.res = 0;
	cmd_eop_edsp_sw_version_register.header.parts.first.bits.chan = 0;
	cmd_eop_edsp_sw_version_register.header.parts.second.eop.bits.mod = VIN_MOD_TEST;
	cmd_eop_edsp_sw_version_register.header.parts.second.eop.bits.ecmd  = VIN_EOP_EDSPSWVERSREG;
	cmd_eop_edsp_sw_version_register.header.parts.second.eop.bits.length = 2;

	if (vin_read(ctx, cmd_eop_edsp_sw_version_register.header, &cmd_eop_edsp_sw_version_register,
			sizeof(struct vin_cmd_eop_edsp_sw_version_register)) < 0) {
		vin_message_stack_printf(ctx, "libvinetic.c:%d in %s vin_write() failed: %s", __LINE__, __PRETTY_FUNCTION__, strerror(errno));
		goto vin_read_fw_version_error;
	}

	memcpy(&ctx->edsp_sw_version_register, &cmd_eop_edsp_sw_version_register.eop_edsp_sw_version_register, sizeof(struct vin_eop_edsp_sw_version_register));

	return 0;

vin_read_fw_version_error:
	return -1;
}

double vin_gainem_to_gaindb(u_int8_t em)
{
	double g;
	u_int8_t e = (em >> 5) & 0x7;
	u_int8_t m = em & 0x1f;
	g = pow(2, (9 - e)) * (32 + m);
	return 24.08 + 20 * log10(g / 32768);
}

u_int8_t vin_gaindb_to_gainem(double g)
{
	if(g > VIN_GAINDB_MAX) g = VIN_GAINDB_MAX;
	if(g < VIN_GAINDB_MIN) g = VIN_GAINDB_MIN;
	//
	double fe = ceil(3 - (g / 6.02));
	double fm = trunc(powf(10, (g / 20)) * pow(2, (2 + fe)) - 32);
	u_int8_t e = ((u_int8_t)fe) & 0x7;
	u_int8_t m = ((u_int8_t)fm) & 0x1f;
	return (e << 5) | m;
}

const char *vin_ali_channel_om_str(unsigned int om)
{
	switch (om) {
		case VIN_OP_MODE_PDHI: return "Power Down High Impedance";
		case VIN_OP_MODE_RP:
		case VIN_OP_MODE_RP1: return "Ring Pause";
		case VIN_OP_MODE_RPHIT: return "Ring Pause HIT";
		case VIN_OP_MODE_RPHIR: return "Ring Pause HIR";
		case VIN_OP_MODE_RPHIRT: return "Ring Pause HIRT";
		case VIN_OP_MODE_RPTG: return "Ring Pause Tip Ground";
		case VIN_OP_MODE_RPRG: return "Ring Pause Ring Ground";
		case VIN_OP_MODE_AH: return "Active High";
		case VIN_OP_MODE_AHIT: return "Active HIT";
		case VIN_OP_MODE_AHIR: return "Active HIR";
		case VIN_OP_MODE_AHIRT: return "Active HIRT";
		case VIN_OP_MODE_AB: return "Active Boost";
		case VIN_OP_MODE_ATG: return "Active Tip Ground";
		case VIN_OP_MODE_ARG: return "Active Ring Ground";
		case VIN_OP_MODE_AHR: return "Active High Resistive";
		case VIN_OP_MODE_AL: return "Active Low";
		case VIN_OP_MODE_AT: return "Active Test";
		case VIN_OP_MODE_ATI: return "Active Test In";
		case VIN_OP_MODE_SPDR: return "Sleep Power Down Resistive";
		case VIN_OP_MODE_GS: return "Ground Start";
		case VIN_OP_MODE_AGS: return "Active Ground Start";
		case VIN_OP_MODE_GSFRP: return "Ground Start Fix Ring";
		case VIN_OP_MODE_R:
		case VIN_OP_MODE_R1: return "Ringing";
		case VIN_OP_MODE_RHIT: return "Ringing HIT";
		case VIN_OP_MODE_RHIR: return "Ringing HIR";
		case VIN_OP_MODE_RTG: return "Ringing Tip Ground";
		case VIN_OP_MODE_RRG: return "Ringing Ring Ground";
		case VIN_OP_MODE_RTDT: return "Ringing Trip Detection Test";
		case VIN_OP_MODE_AHM: return "Active High Metering";
		case VIN_OP_MODE_AMHIT: return "Active Metering HIT";
		case VIN_OP_MODE_AMHIR: return "Active Metering HIR";
		case VIN_OP_MODE_AMTG: return "Active Metering Tip Ground";
		case VIN_OP_MODE_AMRG: return "Active Metering Ring Ground";
		case VIN_OP_MODE_ABM: return "Active Boost Metering";
		case VIN_OP_MODE_ALM: return "Active Low Metering";
		case VIN_OP_MODE_PDRH: return "Power Down Resistive BATH";
		case VIN_OP_MODE_PDRR: return "Power Down Resistive BATR";
		case VIN_OP_MODE_PDA: return "Power Down Active";
		default: return "Reserved";
	}
}

const char *vin_signal_str(unsigned int sig)
{
	switch (sig) {
		case VIN_SIG_NULL: return "null";
		case VIN_SIG_PCM_OUT00: return "pcm00";
		case VIN_SIG_PCM_OUT01: return "pcm01";
		case VIN_SIG_PCM_OUT02: return "pcm02";
		case VIN_SIG_PCM_OUT03: return "pcm03";
		case VIN_SIG_PCM_OUT04: return "pcm04";
		case VIN_SIG_PCM_OUT05: return "pcm05";
		case VIN_SIG_PCM_OUT06: return "pcm06";
		case VIN_SIG_PCM_OUT07: return "pcm07";
		case VIN_SIG_PCM_OUT08: return "pcm08";
		case VIN_SIG_PCM_OUT09: return "pcm09";
		case VIN_SIG_PCM_OUT10: return "pcm10";
		case VIN_SIG_PCM_OUT11: return "pcm11";
		case VIN_SIG_PCM_OUT12: return "pcm12";
		case VIN_SIG_PCM_OUT13: return "pcm13";
		case VIN_SIG_PCM_OUT14: return "pcm14";
		case VIN_SIG_PCM_OUT15: return "pcm15";
		case VIN_SIG_ALM_OUT00: return "alm0";
		case VIN_SIG_ALM_OUT01: return "alm1";
		case VIN_SIG_ALM_OUT02: return "alm2";
		case VIN_SIG_ALM_OUT03: return "alm3";
		case VIN_SIG_COD_OUT00: return "cod0";
		case VIN_SIG_COD_OUT01: return "cod1";
		case VIN_SIG_COD_OUT02: return "cod2";
		case VIN_SIG_COD_OUT03: return "cod3";
		case VIN_SIG_COD_OUT04: return "cod4";
		case VIN_SIG_COD_OUT05: return "cod5";
		case VIN_SIG_COD_OUT06: return "cod6";
		case VIN_SIG_COD_OUT07: return "cod7";
		case VIN_SIG_SIG_OUTA0: return "siga0";
		case VIN_SIG_SIG_OUTB0: return "sigb0";
		case VIN_SIG_SIG_OUTA1: return "siga1";
		case VIN_SIG_SIG_OUTB1: return "sigb1";
		case VIN_SIG_SIG_OUTA2: return "siga2";
		case VIN_SIG_SIG_OUTB2: return "sigb2";
		case VIN_SIG_SIG_OUTA3: return "siga3";
		case VIN_SIG_SIG_OUTB3: return "sigb3";
		default: return "unknown";
	}
}

void vin_state_dump(struct vinetic_context *ctx)
{
	size_t i;

	vin_message_stack_printf(ctx, "Revision: %s\n", vin_revision_str(ctx));
	vin_message_stack_printf(ctx, "EDSP firmware version %u.%u.%u\n",
				(ctx->edsp_sw_version_register.mv << 13) +
				(ctx->edsp_sw_version_register.prt << 12) +
				(ctx->edsp_sw_version_register.features << 0),
				ctx->edsp_sw_version_register.main_version,
				ctx->edsp_sw_version_register.release);
	// SlIC Operation Mode
	for (i = 0; i < 4; i++) {
		vin_message_stack_printf(ctx, "SLIC[%lu] Mode: %s\n", (unsigned long int)i, vin_ali_channel_om_str(ctx->ali_opmode[i]));
	}
	// ALI Module
	vin_message_stack_printf(ctx, "ALI Module: %s\n", (ctx->eop_ali_control.en == VIN_EN)?"enabled":"disabled");
	if (ctx->eop_ali_control.en == VIN_EN) {
		// channels
		for (i = 0; i < 4; i++) {
			vin_message_stack_printf(ctx, "\tChannel[%lu]: %s\n", (unsigned long int)i, (ctx->eop_ali_channel[i].en == VIN_EN)?"enabled":"disabled");
			if (ctx->eop_ali_channel[i].en == VIN_EN) {
				vin_message_stack_printf(ctx, "\t\tgainr=%2.2f\n", vin_gainem_to_gaindb(ctx->eop_ali_channel[i].gainr));
				vin_message_stack_printf(ctx, "\t\tgainx=%2.2f\n", vin_gainem_to_gaindb(ctx->eop_ali_channel[i].gainx));
				if (ctx->eop_ali_channel[i].i1 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti1=%s\n", vin_signal_str(ctx->eop_ali_channel[i].i1));
				if (ctx->eop_ali_channel[i].i2 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti2=%s\n", vin_signal_str(ctx->eop_ali_channel[i].i2));
				if (ctx->eop_ali_channel[i].i3 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti3=%s\n", vin_signal_str(ctx->eop_ali_channel[i].i3));
				if (ctx->eop_ali_channel[i].i4 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti4=%s\n", vin_signal_str(ctx->eop_ali_channel[i].i4));
				if (ctx->eop_ali_channel[i].i5 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti5=%s\n", vin_signal_str(ctx->eop_ali_channel[i].i5));
			}
			vin_message_stack_printf(ctx, "\t\tNELEC: %s\n", (ctx->eop_ali_near_end_lec[i].en == VIN_EN)?"enabled":"disabled");
			if (ctx->eop_ali_near_end_lec[i].en == VIN_EN) {
				vin_message_stack_printf(ctx, "\t\t\tDTM: %u\n", ctx->eop_ali_near_end_lec[i].dtm);
				vin_message_stack_printf(ctx, "\t\t\tOLDC: %u\n", ctx->eop_ali_near_end_lec[i].oldc);
				vin_message_stack_printf(ctx, "\t\t\tAS: %u\n", ctx->eop_ali_near_end_lec[i].as);
				vin_message_stack_printf(ctx, "\t\t\tNLP: %u\n", ctx->eop_ali_near_end_lec[i].nlp);
				vin_message_stack_printf(ctx, "\t\t\tNLPM: %u\n", ctx->eop_ali_near_end_lec[i].nlpm);
				vin_message_stack_printf(ctx, "\t\t\tLECNR: %u\n", ctx->eop_ali_near_end_lec[i].lecnr);
			}
		}
	}

	// Signaling Module
	vin_message_stack_printf(ctx, "Signaling Module: %s\n", (ctx->eop_signaling_control.en == VIN_EN)?"enabled":"disabled");
	if (ctx->eop_signaling_control.en == VIN_EN) {
		// channels
		for (i = 0; i < 4; i++) {
			vin_message_stack_printf(ctx, "\tChannel[%lu]: %s\n", (unsigned long int)i, (ctx->eop_signaling_channel[i].en == VIN_EN)?"enabled":"disabled");
			if (ctx->eop_signaling_channel[i].en == VIN_EN) {
				if (ctx->eop_signaling_channel[i].i1 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti1=%s\n", vin_signal_str(ctx->eop_signaling_channel[i].i1));
				if (ctx->eop_signaling_channel[i].i2 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti2=%s\n", vin_signal_str(ctx->eop_signaling_channel[i].i2));
				vin_message_stack_printf(ctx, "\t\tDTMF Receiver: %s\n", (ctx->eop_dtmf_receiver[i].en == VIN_EN)?"enabled":"disabled");
				if (ctx->eop_dtmf_receiver[i].en == VIN_EN) {
					vin_message_stack_printf(ctx, "\t\t\tET: %u\n", ctx->eop_dtmf_receiver[i].et);
					vin_message_stack_printf(ctx, "\t\t\tIS: %u\n", ctx->eop_dtmf_receiver[i].is);
					vin_message_stack_printf(ctx, "\t\t\tAS: %u\n", ctx->eop_dtmf_receiver[i].as);
					vin_message_stack_printf(ctx, "\t\t\tDTRNR: %u\n", ctx->eop_dtmf_receiver[i].dtrnr);
				}
			}
		}
	}

#if 0
	struct vin_eop_dtmf_receiver eop_dtmf_receiver[4];
	struct vin_eop_utg eop_utg[4];
	struct vin_eop_signaling_channel_configuration_rtp_support eop_signaling_channel_configuration_rtp_support[4];

	struct vin_eop_coder_configuration_rtp_support eop_coder_configuration_rtp_support;
	struct vin_eop_coder_channel_speech_compression eop_coder_channel_speech_compression[4];
	struct vin_eop_coder_channel_configuration_rtp_support eop_coder_channel_configuration_rtp_support[4];
#endif
	// Coder Module
	vin_message_stack_printf(ctx, "Coder Module: %s\n", (ctx->eop_coder_control.en == VIN_EN)?"enabled":"disabled");
	if (ctx->eop_coder_control.en == VIN_EN) {
		// channels
		for (i = 0; i < 4; i++) {
			vin_message_stack_printf(ctx, "\tChannel[%lu]: %s\n", (unsigned long int)i, (ctx->eop_coder_channel_speech_compression[i].en == VIN_EN)?"enabled":"disabled");
			if (ctx->eop_coder_channel_speech_compression[i].en == VIN_EN) {
				vin_message_stack_printf(ctx, "\t\tgain1=%2.2f\n", vin_gainem_to_gaindb(ctx->eop_coder_channel_speech_compression[i].gain1));
				vin_message_stack_printf(ctx, "\t\tgain2=%2.2f\n", vin_gainem_to_gaindb(ctx->eop_coder_channel_speech_compression[i].gain2));
				if (ctx->eop_coder_channel_speech_compression[i].i1 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti1=%s\n", vin_signal_str(ctx->eop_coder_channel_speech_compression[i].i1));
				if (ctx->eop_coder_channel_speech_compression[i].i2 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti2=%s\n", vin_signal_str(ctx->eop_coder_channel_speech_compression[i].i2));
				if (ctx->eop_coder_channel_speech_compression[i].i3 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti3=%s\n", vin_signal_str(ctx->eop_coder_channel_speech_compression[i].i3));
				if (ctx->eop_coder_channel_speech_compression[i].i4 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti4=%s\n", vin_signal_str(ctx->eop_coder_channel_speech_compression[i].i4));
				if (ctx->eop_coder_channel_speech_compression[i].i5 != VIN_SIG_NULL) vin_message_stack_printf(ctx, "\t\ti5=%s\n", vin_signal_str(ctx->eop_coder_channel_speech_compression[i].i5));
				vin_message_stack_printf(ctx, "\t\tCODNR=%u\n", ctx->eop_coder_channel_speech_compression[i].codnr);
			}
		}
	}
}

/******************************************************************************/
/* end of libvinetic.c                                                        */
/******************************************************************************/
