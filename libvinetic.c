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

#include "libvinetic.h"

#define PRINTF(_fmt, _args...) \
	do { \
		fprintf(stdout, _fmt, ##_args); \
		fflush(stdout); \
	} while (0)

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
		memcpy(&ctx->eop_coder_channel_configuration_rtp_support[i],
				&default_eop_coder_channel_configuration_rtp_support,
				sizeof(struct vin_eop_coder_channel_configuration_rtp_support));
	}
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
	}

	return res;
}

int vin_open(struct vinetic_context *ctx)
{
	ctx->dev_fd = open(ctx->dev_path, O_RDWR);
	ctx->error = errno;
	return ctx->dev_fd;
}

void vin_close(struct vinetic_context *ctx)
{
	close(ctx->dev_fd);
	ctx->dev_fd = -1;
}

int vin_reset(struct vinetic_context *ctx)
{
	int rc = ioctl(ctx->dev_fd, VINETIC_RESET, NULL);
	ctx->error = errno;
	return rc;
}

int vin_reset_rdyq(struct vinetic_context *ctx)
{
	int rc = ioctl(ctx->dev_fd, VINETIC_RESET_RDYQ, NULL);
	ctx->error = errno;
	return rc;
}

int vin_flush_mbox(struct vinetic_context *ctx)
{
	int rc = ioctl(ctx->dev_fd, VINETIC_FLUSH_MBOX, NULL);
	ctx->error = errno;
	return rc;
}

int vin_is_not_ready(struct vinetic_context *ctx)
{
	int not_ready;
	if (ioctl(ctx->dev_fd, VINETIC_GET_NOT_READY, &not_ready) < 0) {
		ctx->error = errno;
		not_ready = -1;
	}
	return not_ready;
}

u_int16_t vin_read_dia(struct vinetic_context *ctx)
{
	u_int16_t dia;
	if (ioctl(ctx->dev_fd, VINETIC_READ_DIA, &dia) < 0) {
		ctx->error = errno;
		dia = 0xffff;
	}
	return dia;
}

int vin_resync(struct vinetic_context *ctx)
{
	int rc;
	union vin_cmd_short cmd_short;

	// Re-SYNChronize PCM clock
	cmd_short.full = VIN_wRESYNC;
	rc = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short));
	ctx->errorline = __LINE__ - 1;
	ctx->error = errno;
	return rc;
}

int vin_cerr_acknowledge(struct vinetic_context *ctx)
{
	int rc;
	off64_t lsrc;
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
	if ((lsrc = lseek64(ctx->dev_fd, cmd.full, SEEK_SET)) < 0) {
		ctx->error = errno;
// 		PRINTF("lseek()=%ld\n", (long int)lsrc);
		rc = (int)lsrc;
		goto vin_cerr_acknowledge_end;
	}
	if ((rc = write(ctx->dev_fd, &cmd.full, 0)) < 0) {
		ctx->error = errno;
// 		PRINTF("write()=%ld\n", (long int)rc);
		goto vin_cerr_acknowledge_end;
	}

vin_cerr_acknowledge_end:
	return rc;
}

int vin_poll_set(struct vinetic_context *ctx, int poll)
{
	int rc = ioctl(ctx->dev_fd, VINETIC_SET_POLL, &poll);
	ctx->error = errno;
	return rc;
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

char *vin_error_str(struct vinetic_context *ctx)
{
	return strerror(ctx->error);
}

char *vin_revision_str(struct vinetic_context *ctx)
{
	switch (ctx->revision)
	{
		case VIN_REV_13: return "1.3";
		case VIN_REV_14: return "1.4";
		default: return "unknown";
	}
}

int vin_reset_status(struct vinetic_context *ctx)
{
	int rc = ioctl(ctx->dev_fd, VINETIC_RESET_STATUS, NULL);
	ctx->errorline = __LINE__ - 1;
	ctx->error = errno;
	return rc;
}

ssize_t vin_write(struct vinetic_context *ctx, size_t track_err, const void *buf, size_t count)
{
	ssize_t rc;
	off64_t lsrc;
	union vin_cmd cmd;
	u_int8_t *data = (u_int8_t *)buf;
	size_t length;
	union vin_cmd auxcmd;
	struct vin_read_bxsr bxsr;

	cmd.full = 0;
	if (count < 2) {
		rc = -EINVAL;
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
// 	PRINTF("%04x\n", bxsr.bxsr1.full);
// 	PRINTF("%04x\n", bxsr.bxsr2.full);
	if (bxsr.bxsr2.bits.host_err || bxsr.bxsr2.bits.pibx_of || bxsr.bxsr2.bits.cibx_of) {
// 		PRINTF("host_err=%u\n", bxsr.bxsr2.bits.host_err);
// 		PRINTF("pibx_of=%u\n", bxsr.bxsr2.bits.pibx_of);
// 		PRINTF("cibx_of=%u\n", bxsr.bxsr2.bits.cibx_of);
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
	if ((lsrc = lseek64(ctx->dev_fd, cmd.full, SEEK_SET)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
// 		PRINTF("lseek()=%ld\n", (long int)lsrc);
		rc = (ssize_t)lsrc;
		goto vin_write_end;
	}
	if ((rc = write(ctx->dev_fd, data+4, length)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
// 		PRINTF("write()=%ld\n", (long int)rc);
		goto vin_write_end;
	}
	if (track_err)
		usleep(130);
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
	if (bxsr.bxsr1.bits.cerr) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno = ENOMSG;
#if 0
		if (vin_cerr_acknowledge(ctx) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_write_end;
		}
#endif
		rc = -1;
		goto vin_write_end;
	}

vin_write_end:
	return rc;
}

ssize_t vin_read(struct vinetic_context *ctx, union vin_cmd cmd, void *buf, size_t count)
{
	ssize_t rc;
	off64_t lsrc;
	union vin_cmd auxcmd;
	struct vin_read_bxsr bxsr;

	// check mailbox status
	auxcmd.full = 0;
	auxcmd.parts.first.full = VIN_rBXSR;
	if ((lsrc = lseek64(ctx->dev_fd, auxcmd.full, SEEK_SET)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		rc = (ssize_t)lsrc;
		goto vin_read_end;
	}
	if ((rc = read(ctx->dev_fd, &bxsr, sizeof(struct vin_read_bxsr))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_read_end;
	}
// 	PRINTF("%04x\n", bxsr.bxsr1.full);
// 	PRINTF("%04x\n", bxsr.bxsr2.full);
	if (bxsr.bxsr2.bits.host_err || bxsr.bxsr2.bits.pibx_of || bxsr.bxsr2.bits.cibx_of) {
// 		PRINTF("host_err=%u\n", bxsr.bxsr2.bits.host_err);
// 		PRINTF("pibx_of=%u\n", bxsr.bxsr2.bits.pibx_of);
// 		PRINTF("cibx_of=%u\n", bxsr.bxsr2.bits.cibx_of);
		// acknowledge error
		auxcmd.full = 0;
		auxcmd.parts.first.full = VIN_wPHIERR;
		if ((lsrc = lseek64(ctx->dev_fd, auxcmd.full, SEEK_SET)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			rc = (ssize_t)lsrc;
			goto vin_read_end;
		}
		if ((rc = write(ctx->dev_fd, &auxcmd.full, 0)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_read_end;
		}
		// wait for recovery
		usleep(500);
	}
	// base read command
	if ((lsrc = lseek64(ctx->dev_fd, cmd.full, SEEK_SET)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		rc = (ssize_t)lsrc;
		goto vin_read_end;
	}
	if ((rc = read(ctx->dev_fd, buf, count)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_read_end;
	}

vin_read_end:
	return rc;
}

ssize_t vin_get_status(struct vinetic_context *ctx)
{
	ssize_t rc;
	off64_t lsrc;

	memcpy(&ctx->status_old, &ctx->status, sizeof(struct vin_status_registers));

	// read command
	if ((lsrc = lseek64(ctx->dev_fd, 0xffffffff, SEEK_SET)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		rc = (ssize_t)lsrc;
		goto vin_get_status_end;
	}
	if ((rc = read(ctx->dev_fd, &ctx->status, sizeof(struct vin_status_registers))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_get_status_end;
	}

vin_get_status_end:
	return rc;
}

u_int16_t vin_phi_revision(struct vinetic_context *ctx)
{
	u_int16_t rev;

	if (ioctl(ctx->dev_fd, VINETIC_REVISION, &rev) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		rev = 0;
	}
	ctx->revision = rev;
	return rev;
}

u_int16_t vin_phi_checksum(struct vinetic_context *ctx)
{
	u_int16_t csum;

	if (ioctl(ctx->dev_fd, VINETIC_CHECKSUM, &csum) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		csum = 0;
	}

	return csum;
}

int vin_phi_disable_interrupt(struct vinetic_context *ctx)
{
	int rc = ioctl(ctx->dev_fd, VINETIC_DISABLE_IRQ, NULL);
	ctx->errorline = __LINE__ - 1;
	ctx->error = errno;
	return rc;
}

int vin_check_mbx_empty(struct vinetic_context *ctx)
{
	ssize_t rc;
	size_t cnt;
	union vin_cmd cmd;
	struct vin_read_bxsr bxsr;

	rc = 0;
	cnt = 255;
	cmd.full = 0;
	cmd.parts.first.full = VIN_rBXSR;

	for (;;)
	{
		if ((rc = vin_read(ctx, cmd, &bxsr, sizeof(struct vin_read_bxsr))) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_check_mbx_empty_end;
		}
// 		PRINTF("mbx_empty=%u\n", bxsr.bxsr2.bits.mbx_empty);
		if (bxsr.bxsr2.bits.mbx_empty) break;
		if (!cnt--) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno = EIO;
			rc = -EIO;
			goto vin_check_mbx_empty_end;
		}
		usleep(1000);
	}

vin_check_mbx_empty_end:
	return rc;
}

int vin_wait_dl_rdy(struct vinetic_context *ctx)
{
	ssize_t rc;
	size_t cnt;
	union vin_cmd cmd;
	struct vin_read_hwsr hwsr;

	rc = 0;
	cmd.full = 0;
	cmd.parts.first.full = VIN_rHWSR;
	cnt = 8000;
	for (;;)
	{
		if ((rc = vin_read(ctx, cmd, &hwsr, sizeof(struct vin_read_hwsr))) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_wait_dl_ready_end;
		}
		if (hwsr.hwsr2.bits.dl_rdy) break;
		if (!cnt--) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = -EIO;
			rc = -EIO;
			goto vin_wait_dl_ready_end;
		}
		usleep(125);
	}

vin_wait_dl_ready_end:
	return rc;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	// Maximazing Command In-Box
	cmd_short.full = VIN_wMAXCBX;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	// Load EDSP Micro Program
	cmd_short.full = VIN_wLEMP;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	// open PRAM file
	if ((fd = open(ctx->pram_path, O_RDONLY)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	if (fstat(fd, &fd_stat) < 0) {
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
		if ((res = vin_write(ctx, 0, &cmd_eop_set_pram_address, sizeof(union vin_cmd) + sizeof(u_int16_t)*2)) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
		while (seg_size)
		{
			// Check for MBX-EMPTY
			if (vin_check_mbx_empty(ctx) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_edsp_firmware_error;
			}
			// Read PRAM segment data
			if (lseek(fd, fd_offset, SEEK_SET) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_edsp_firmware_error;
			}
			seg_chunk_size = (seg_size < 252) ? (seg_size) : (252);
// 			PRINTF("pram: %lu %lu\n", (unsigned long int)seg_chunk_size, (unsigned long int)seg_chunk_size);
			if (read(fd, cmd_eop_access_pram.data, seg_chunk_size*2) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_edsp_firmware_error;
			}
			fd_offset += seg_chunk_size*2;
			for (i=0; i<seg_chunk_size; i++)
#if 0
				cmd_eop_access_pram.data[i] = be16toh(cmd_eop_access_pram.data[i]);
#else
				cmd_eop_access_pram.data[i] = ntohs(cmd_eop_access_pram.data[i]);
#endif
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
			if ((res = vin_write(ctx, 0, &cmd_eop_access_pram, sizeof(union vin_cmd) + seg_chunk_size*2)) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}

	// open DRAM file
	if ((fd = open(ctx->dram_path, O_RDONLY)) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	if (fstat(fd, &fd_stat) < 0) {
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
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_edsp_firmware_error;
		}
#if 0
		seg_size = be32toh(xram_segment_header.size);
#else
		seg_size = ntohl(xram_segment_header.size);
#endif
		while (seg_size)
		{
			// Check for MBX-EMPTY
			if (vin_check_mbx_empty(ctx) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_edsp_firmware_error;
			}
			// Read DRAM segment data
			if (lseek(fd, fd_offset, SEEK_SET) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_edsp_firmware_error;
			}
			seg_chunk_size = (seg_size < 252) ? (seg_size) : (252);
// 			PRINTF("dram: %lu %lu\n", (unsigned long int)seg_chunk_size, (unsigned long int)seg_chunk_size);
			if (read(fd, cmd_eop_access_dram.data, seg_chunk_size*2) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_edsp_firmware_error;
			}
			fd_offset += seg_chunk_size*2;
			for (i=0; i<seg_chunk_size; i++)
#if 0
				cmd_eop_access_dram.data[i] = be16toh(cmd_eop_access_dram.data[i]);
#else
				cmd_eop_access_dram.data[i] = ntohs(cmd_eop_access_dram.data[i]);
#endif
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
			if ((res = vin_write(ctx, 0, &cmd_eop_access_dram, sizeof(union vin_cmd) + seg_chunk_size*2)) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	// Wait for DL-RDY
	if (vin_wait_dl_rdy(ctx) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	// Check for MBX-EMPTY
	if (vin_check_mbx_empty(ctx) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}
	// Minimazing Command In-Box
	cmd_short.full = VIN_wMINCBX;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_edsp_firmware_error;
	}

	// Start EDSP 
	cmd_short.full = VIN_wSTEDSP;
	if ((res = vin_write(ctx, 0, &cmd_short.full, sizeof(union vin_cmd_short))) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_alm_dsp_error;
			}
		} else {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_alm_dsp_error;
		}
		addr_start = tmp_u32;
		// get end address
		if (fgets(fpbuf, sizeof(fpbuf), fp)) {
			if (sscanf(fpbuf, "0x%08X", &tmp_u32) != 1) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_alm_dsp_error;
			}
		} else {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_alm_dsp_error;
		}
		addr_end = tmp_u32;
	} else {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_alm_dsp_error;
	}
// 	PRINTF("\naddr start=0x%08x\n", addr_start);
// 	PRINTF("addr end=0x%08x\n", addr_end);

	// get data
	data_size = (addr_end - addr_start + 1);
// 	PRINTF("data size=%lu\n", data_size);
	while (fgets(fpbuf, sizeof(fpbuf), fp))
	{
		if (!strncasecmp(fpbuf, "[DATA]", strlen("[DATA]"))) {
			flag_data = 1;
			break;
		}
	}
	if (flag_data) {
		// alloc data buffer
		if ((data = malloc(data_size * 2))) {
			datap = data;
			//
			for (i=0; i<data_size; i++)
			{
				if (fgets(fpbuf, sizeof(fpbuf), fp)) {
					if (sscanf(fpbuf, "0x%04X", &tmp_u32) != 1) {
						ctx->errorline = __LINE__ - 1;
						ctx->error = errno;
						goto vin_download_alm_dsp_error;
					}
				} else {
					ctx->errorline = __LINE__ - 1;
					ctx->error = errno;
					goto vin_download_alm_dsp_error;
				}
			*datap++ = tmp_u32 & 0xffff;
			}
		} else {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_alm_dsp_error;
		}
	}

	// get checksum
	while (fgets(fpbuf, sizeof(fpbuf), fp))
	{
		if (!strncasecmp(fpbuf, "[CHECKSUM]", strlen("[CHECKSUM]"))) {
			flag_checksum = 1;
			break;
		}
	}
	if (flag_checksum) {
		if (fgets(fpbuf, sizeof(fpbuf), fp)) {
			if (sscanf(fpbuf, "0x%04X", &tmp_u32) != 1) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_alm_dsp_error;
			}
		} else {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_alm_dsp_error;
		}
// 		checksum = tmp_u32 & 0xffff;
	}
// 	PRINTF("checksum=0x%04x\n", checksum);

	// get dschkr
	while (fgets(fpbuf, sizeof(fpbuf), fp))
	{
		if (!strncasecmp(fpbuf, "[DSCHKR]", strlen("[DSCHKR]"))) {
			flag_dschkr = 1;
			break;
		}
	}
	if (flag_dschkr) {
		if (fgets(fpbuf, sizeof(fpbuf), fp)) {
			if (sscanf(fpbuf, "0x%04X", &tmp_u32) != 1) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
				goto vin_download_alm_dsp_error;
			}
		} else {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_alm_dsp_error;
		}
// 		dschkr = tmp_u32 & 0xffff;
	}
// 	PRINTF("dschkr=0x%04x\n", dschkr);

	// Check for MBX-EMPTY
	if (vin_check_mbx_empty(ctx) < 0) {
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
	while (data_size)
	{
		// Check for MBX-EMPTY
		if (vin_check_mbx_empty(ctx) < 0) {
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
			goto vin_download_alm_dsp_error;
		}
		data_chunk_size = (data_size < 29) ? (data_size) : (29);
		for (i=0; i<data_chunk_size; i++)
			memcpy(&cmd_eop_access_fpi_memory.data[i], datap++, sizeof(u_int16_t));
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
			ctx->errorline = __LINE__ - 1;
			ctx->error = errno;
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
		ctx->error = errno;
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
		goto vin_download_cram_error;
	}

	while (fgets(fpbuf, sizeof(fpbuf), fp))
	{
		res = sscanf(fpbuf, " %[A-Z0-9_] = 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X", hdr, &cfd[0], &cfd[1], &cfd[2], &cfd[3], &cfd[4], &cfd[5]);
		if (res == 7) {
			// Check for MBX-EMPTY
			if (vin_check_mbx_empty(ctx) < 0) {
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
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
				ctx->errorline = __LINE__ - 1;
				ctx->error = errno;
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
		ctx->error = errno;
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
		ctx->error = errno;
		goto vin_set_opmode_error;
	}
	return 0;

vin_set_opmode_error:
	return -1;
}

int vin_ali_control(struct vinetic_context *ctx)
{
	struct vin_cmd_eop_ali_control cmd_eop_ali_control;

	cmd_eop_ali_control.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_ali_control.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_ali_control.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_ali_control.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_ali_control.header.parts.first.bits.res = 0;
	cmd_eop_ali_control.header.parts.first.bits.chan = 0;
	cmd_eop_ali_control.header.parts.second.eop.bits.mod = VIN_MOD_ALI;
	cmd_eop_ali_control.header.parts.second.eop.bits.ecmd  = VIN_EOP_ALI_CONT;
	cmd_eop_ali_control.header.parts.second.eop.bits.length = sizeof(struct vin_eop_ali_control)/2;
	memcpy(&cmd_eop_ali_control.eop_ali_control, &ctx->eop_ali_control, sizeof(struct vin_eop_ali_control));
	if (vin_write(ctx, 1, &cmd_eop_ali_control, sizeof(struct vin_cmd_eop_ali_control)) < 0) {
		ctx->error = errno;
		goto vin_ali_control_error;
	}
	return 0;

vin_ali_control_error:
	return -1;
}

int vin_ali_channel(struct vinetic_context *ctx, unsigned int ch)
{
	struct vin_cmd_eop_ali_channel cmd_eop_ali_channel;

	cmd_eop_ali_channel.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_ali_channel.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_ali_channel.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_ali_channel.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_ali_channel.header.parts.first.bits.res = 0;
	cmd_eop_ali_channel.header.parts.first.bits.chan = ch;
	cmd_eop_ali_channel.header.parts.second.eop.bits.mod = VIN_MOD_ALI;
	cmd_eop_ali_channel.header.parts.second.eop.bits.ecmd  = VIN_EOP_ALI_CHAN;
	cmd_eop_ali_channel.header.parts.second.eop.bits.length = sizeof(struct vin_eop_ali_channel)/2;
	memcpy(&cmd_eop_ali_channel.eop_ali_channel, &ctx->eop_ali_channel[ch], sizeof(struct vin_eop_ali_channel));
	if (vin_write(ctx, 1, &cmd_eop_ali_channel, sizeof(struct vin_cmd_eop_ali_channel)) < 0) {
		ctx->error = errno;
		goto vin_ali_channel_error;
	}
	return 0;

vin_ali_channel_error:
	return -1;
}

int vin_ali_near_end_lec(struct vinetic_context *ctx, unsigned int ch)
{
	struct vin_cmd_eop_ali_near_end_lec cmd_eop_ali_near_end_lec;

	cmd_eop_ali_near_end_lec.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.res = 0;
	cmd_eop_ali_near_end_lec.header.parts.first.bits.chan = ch;
	cmd_eop_ali_near_end_lec.header.parts.second.eop.bits.mod = VIN_MOD_ALI;
	cmd_eop_ali_near_end_lec.header.parts.second.eop.bits.ecmd  = VIN_EOP_ALI_NEAR_END_LEC;
	cmd_eop_ali_near_end_lec.header.parts.second.eop.bits.length = sizeof(struct vin_eop_ali_near_end_lec)/2;
	memcpy(&cmd_eop_ali_near_end_lec.eop_ali_near_end_lec, &ctx->eop_ali_near_end_lec[ch], sizeof(struct vin_eop_ali_near_end_lec));
	if (vin_write(ctx, 1, &cmd_eop_ali_near_end_lec, sizeof(struct vin_cmd_eop_ali_near_end_lec)) < 0) {
		ctx->error = errno;
		goto vin_ali_near_end_lec_error;
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
	cmd_eop_signaling_control.header.parts.second.eop.bits.length = sizeof(struct vin_eop_signaling_control)/2;
	memcpy(&cmd_eop_signaling_control.eop_signaling_control, &ctx->eop_signaling_control, sizeof(struct vin_eop_signaling_control));
	if (vin_write(ctx, 1, &cmd_eop_signaling_control, sizeof(struct vin_cmd_eop_signaling_control)) < 0) {
		ctx->error = errno;
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
	cmd_eop_signaling_channel.header.parts.second.eop.bits.length = sizeof(struct vin_eop_signaling_channel)/2;
	memcpy(&cmd_eop_signaling_channel.eop_signaling_channel, &ctx->eop_signaling_channel[ch], sizeof(struct vin_eop_signaling_channel));
	if (vin_write(ctx, 1, &cmd_eop_signaling_channel, sizeof(struct vin_cmd_eop_signaling_channel)) < 0) {
		ctx->error = errno;
		goto vin_signaling_channel_error;
	}
	return 0;

vin_signaling_channel_error:
	return -1;
}

int vin_dtmf_receiver(struct vinetic_context *ctx, unsigned int ch)
{
	struct vin_cmd_eop_dtmf_receiver cmd_eop_dtmf_receiver;

	cmd_eop_dtmf_receiver.header.parts.first.bits.rw = VIN_WRITE;
	cmd_eop_dtmf_receiver.header.parts.first.bits.sc = VIN_SC_NO;
	cmd_eop_dtmf_receiver.header.parts.first.bits.bc = VIN_BC_NO;
	cmd_eop_dtmf_receiver.header.parts.first.bits.cmd = VIN_CMD_EOP;
	cmd_eop_dtmf_receiver.header.parts.first.bits.res = 0;
	cmd_eop_dtmf_receiver.header.parts.first.bits.chan = ch;
	cmd_eop_dtmf_receiver.header.parts.second.eop.bits.mod = VIN_MOD_SIG;
	cmd_eop_dtmf_receiver.header.parts.second.eop.bits.ecmd  = VIN_EOP_DTMFREC;
	cmd_eop_dtmf_receiver.header.parts.second.eop.bits.length = sizeof(struct vin_eop_dtmf_receiver)/2;
	memcpy(&cmd_eop_dtmf_receiver.eop_dtmf_receiver, &ctx->eop_dtmf_receiver[ch], sizeof(struct vin_eop_dtmf_receiver));
	if (vin_write(ctx, 1, &cmd_eop_dtmf_receiver, sizeof(struct vin_cmd_eop_dtmf_receiver)) < 0) {
		ctx->error = errno;
		goto vin_dtmf_receiver_error;
	}
	return 0;

vin_dtmf_receiver_error:
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
	cmd_eop_signaling_channel_configuration_rtp_support.header.parts.second.eop.bits.length = sizeof(struct vin_eop_signaling_channel_configuration_rtp_support)/2;
	memcpy(&cmd_eop_signaling_channel_configuration_rtp_support.eop_signaling_channel_configuration_rtp_support,
			&ctx->eop_signaling_channel_configuration_rtp_support[ch],
			sizeof(struct vin_eop_signaling_channel_configuration_rtp_support));
	if (vin_write(ctx, 1, &cmd_eop_signaling_channel_configuration_rtp_support, sizeof(struct vin_cmd_eop_signaling_channel_configuration_rtp_support)) < 0) {
		ctx->error = errno;
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
	cmd_eop_coder_control.header.parts.second.eop.bits.length = sizeof(struct vin_eop_coder_control)/2;
	memcpy(&cmd_eop_coder_control.eop_coder_control, &ctx->eop_coder_control, sizeof(struct vin_eop_coder_control));
	if (vin_write(ctx, 1, &cmd_eop_coder_control, sizeof(struct vin_cmd_eop_coder_control)) < 0) {
		ctx->error = errno;
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
	cmd_eop_coder_channel_speech_compression.header.parts.second.eop.bits.length = sizeof(struct vin_eop_coder_channel_speech_compression)/2;
	memcpy(&cmd_eop_coder_channel_speech_compression.eop_coder_channel_speech_compression,
			&ctx->eop_coder_channel_speech_compression[ch],
			sizeof(struct vin_eop_coder_channel_speech_compression));
	if (vin_write(ctx, 1, &cmd_eop_coder_channel_speech_compression, sizeof(struct vin_cmd_eop_coder_channel_speech_compression)) < 0) {
		ctx->error = errno;
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
	cmd_eop_coder_configuration_rtp_support.header.parts.second.eop.bits.length = sizeof(struct vin_eop_coder_configuration_rtp_support)/2;
	memcpy(&cmd_eop_coder_configuration_rtp_support.eop_coder_configuration_rtp_support,
			&ctx->eop_coder_configuration_rtp_support,
			sizeof(struct vin_eop_coder_configuration_rtp_support));
	if (vin_write(ctx, 1, &cmd_eop_coder_configuration_rtp_support, sizeof(struct vin_cmd_eop_coder_configuration_rtp_support)) < 0) {
		ctx->error = errno;
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
	cmd_eop_coder_channel_configuration_rtp_support.header.parts.second.eop.bits.length = sizeof(struct vin_eop_coder_channel_configuration_rtp_support)/2;
	memcpy(&cmd_eop_coder_channel_configuration_rtp_support.eop_coder_channel_configuration_rtp_support,
			&ctx->eop_coder_channel_configuration_rtp_support[ch],
			sizeof(struct vin_eop_coder_channel_configuration_rtp_support));
	if (vin_write(ctx, 1, &cmd_eop_coder_channel_configuration_rtp_support, sizeof(struct vin_cmd_eop_coder_channel_configuration_rtp_support)) < 0) {
		ctx->error = errno;
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
		ctx->error = errno;
		goto vin_coder_channel_jb_statistic_reset_error;
	}
	return 0;

vin_coder_channel_jb_statistic_reset_error:
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
		ctx->errorline = __LINE__ - 1;
		ctx->error = errno;
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
		ctx->error = errno;
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

/******************************************************************************/
/* end of libvinetic.c                                                        */
/******************************************************************************/
