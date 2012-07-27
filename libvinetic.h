/******************************************************************************/
/* libvinetic.h                                                               */
/******************************************************************************/

#ifndef __LIBVINETIC__H__
#define __LIBVINETIC_H__

#include <arpa/inet.h>

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

	char *dev_name;

	int error;
	int errorline;

	u_int16_t revision;

	// status
	struct vin_status_registers status;
	struct vin_status_registers status_old;
	struct vin_status_registers status_mask;

	struct vin_eop_ali_control eop_ali_control;
	struct vin_eop_ali_channel eop_ali_channel[4];
	struct vin_eop_ali_near_end_lec eop_ali_near_end_lec[4];

	struct vin_eop_coder_control eop_coder_control;
	struct vin_eop_coder_channel_speech_compression eop_coder_channel_speech_compression[4];
	struct vin_eop_coder_channel_configuration_rtp_support eop_coder_channel_configuration_rtp_support[4];

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

extern int vin_reset_status(struct vinetic_context *ctx);
extern ssize_t vin_get_status(struct vinetic_context *ctx);

extern int vin_resync(struct vinetic_context *ctx);
extern int vin_cerr_acknowledge(struct vinetic_context *ctx);

extern int vin_poll_set(struct vinetic_context *ctx, int poll);

extern char *vin_get_dev_name(struct vinetic_context *ctx);
extern void vin_set_dev_name(struct vinetic_context *ctx, char *name);

extern char *vin_error_str(struct vinetic_context *ctx);

#define VIN_REV_13 0x2442
#define VIN_REV_14 0x2484
extern char *vin_revision_str(struct vinetic_context *ctx);


extern int vin_read_fw_version(struct vinetic_context *ctx);

extern ssize_t vin_write(struct vinetic_context *ctx, int track_err, const void *buf, size_t count);

extern ssize_t vin_read(struct vinetic_context *ctx, union vin_cmd cmd, void *buf, size_t count);

extern u_int16_t vin_phi_revision(struct vinetic_context *ctx);
extern u_int16_t vin_phi_checksum(struct vinetic_context *ctx);
extern int vin_phi_disable_interrupt(struct vinetic_context *ctx);

extern int vin_download_edsp_firmware(struct vinetic_context *ctx);

extern int vin_download_alm_dsp(struct vinetic_context *ctx, char *path);
extern int vin_jump_alm_dsp(struct vinetic_context *ctx, unsigned int chan);

extern int vin_download_cram(struct vinetic_context *ctx, unsigned int chan, char *path);

extern int vin_write_sop_generic(struct vinetic_context *ctx, unsigned int chan, u_int16_t offset, u_int16_t data);

extern int vin_set_endian_mode(struct vinetic_context *ctx, int mode);

extern int vin_set_opmode(struct vinetic_context *ctx, unsigned int ch, unsigned int mode);

#define vin_poll_enable(_ctx) \
	({ \
		int __res = vin_poll_set(_ctx, 1); \
		__res; \
	})

#define vin_poll_disable(_ctx) \
	({ \
		int __res = vin_poll_set(_ctx, 0); \
		__res; \
	})

#define vin_set_little_endian_mode(_ctx) \
	({ \
		int __res = vin_set_endian_mode(_ctx, VIN_LITTLE_ENDIAN); \
		__res; \
	})

extern int vin_ali_control(struct vinetic_context *ctx);

#define vin_ali_enable(_ctx) \
	({ \
		_ctx.eop_ali_control.en = VIN_EN; \
		int __res = vin_ali_control(&_ctx); \
		if (__res < 0) _ctx.eop_ali_control.en = VIN_DIS; \
		__res; \
	})

#define vin_ali_disable(_ctx) \
	({ \
		_ctx.eop_ali_control.en = VIN_DIS; \
		int __res = vin_ali_control(&_ctx); \
		if (__res < 0) _ctx.eop_ali_control.en = VIN_EN; \
		__res; \
	})

#define is_vin_ali_enabled(_ctx) \
	({ \
		int __res = 0; \
		if (_ctx.eop_ali_control.en == VIN_EN) \
			__res = 1; \
		__res; \
	})

#define is_vin_ali_used(_ctx) \
	({ \
		size_t __i; \
		int __res = 0; \
		for (__i=0; __i<4; __i++) \
		{ \
			if (_ctx.eop_ali_channel[__i].en == VIN_EN) { \
				__res = 1; \
				break; \
			} \
		} \
		__res; \
	})

extern int vin_ali_channel(struct vinetic_context *ctx, unsigned int ch);

#define vin_ali_channel_enable(_ctx, _ch) \
	({ \
		_ctx.eop_ali_channel[_ch].en = VIN_EN; \
		int __res = vin_ali_channel(&_ctx, _ch); \
		if (__res < 0) _ctx.eop_ali_channel[_ch].en = VIN_DIS; \
		__res; \
	})

#define vin_ali_channel_disable(_ctx, _ch) \
	({ \
		_ctx.eop_ali_channel[_ch].en = VIN_DIS; \
		int __res = vin_ali_channel(&_ctx, _ch); \
		if (__res < 0) _ctx.eop_ali_channel[_ch].en = VIN_EN; \
		__res; \
	})

#define is_vin_ali_channel_enabled(_ctx, _ch) \
	({ \
		int __res = 0; \
		if (_ctx.eop_ali_channel[_ch].en == VIN_EN) \
			__res = 1; \
		__res; \
	})

#define vin_ali_channel_set_gainr(_ctx, _ch, _gainr) \
	do { \
		_ctx.eop_ali_channel[_ch].gainr = _gainr; \
	} while (0)

#define vin_ali_channel_set_gainx(_ctx, _ch, _gainx) \
	do { \
		_ctx.eop_ali_channel[_ch].gainx = _gainx; \
	} while (0)

#define vin_ali_channel_set_input_coder(_ctx, _ch, _inp, _cod) \
	do { \
		if (_inp == 5) \
			_ctx.eop_ali_channel[_ch].i5 = VIN_SIG_COD_OUT00 + _cod; \
		else if (_inp == 4) \
			_ctx.eop_ali_channel[_ch].i4 = VIN_SIG_COD_OUT00 + _cod; \
		else if (_inp == 3) \
			_ctx.eop_ali_channel[_ch].i3 = VIN_SIG_COD_OUT00 + _cod; \
		else if (_inp == 2) \
			_ctx.eop_ali_channel[_ch].i2 = VIN_SIG_COD_OUT00 + _cod; \
		else \
			_ctx.eop_ali_channel[_ch].i1 = VIN_SIG_COD_OUT00 + _cod; \
	} while (0)

extern int vin_ali_near_end_lec(struct vinetic_context *ctx, unsigned int ch);

#define vin_ali_near_end_lec_enable(_ctx, _ch) \
	({ \
		_ctx.eop_ali_near_end_lec[_ch].en = VIN_EN; \
		_ctx.eop_ali_near_end_lec[_ch].lecnr = _ch; \
		int __res = vin_ali_near_end_lec(&_ctx, _ch); \
		if (__res < 0) _ctx.eop_ali_near_end_lec[_ch].en = VIN_DIS; \
		__res; \
	})

#define vin_ali_near_end_lec_disable(_ctx, _ch) \
	({ \
		_ctx.eop_ali_near_end_lec[_ch].en = VIN_DIS; \
		int __res = vin_ali_near_end_lec(&_ctx, _ch); \
		if (__res < 0) _ctx.eop_ali_near_end_lec[_ch].en = VIN_EN; \
		__res; \
	})

#define is_vin_ali_near_end_lec_enabled(_ctx, _ch) \
	({ \
		int __res = 0; \
		if (_ctx.eop_ali_near_end_lec[_ch].en == VIN_EN) \
			__res = 1; \
		__res; \
	})

#define vin_ali_near_end_lec_set_dtm(_ctx, _ch, _dtm) \
	do { \
		_ctx.eop_ali_near_end_lec[_ch].dtm = _dtm; \
	} while (0)

#define vin_ali_near_end_lec_set_oldc(_ctx, _ch, _oldc) \
	do { \
		_ctx.eop_ali_near_end_lec[_ch].oldc = _oldc; \
	} while (0)

#define vin_ali_near_end_lec_set_as(_ctx, _ch, _as) \
	do { \
		_ctx.eop_ali_near_end_lec[_ch].as = _as; \
	} while (0)

#define vin_ali_near_end_lec_set_nlp(_ctx, _ch, _nlp) \
	do { \
		_ctx.eop_ali_near_end_lec[_ch].nlp = _nlp; \
	} while (0)

#define vin_ali_near_end_lec_set_nlpm(_ctx, _ch, _nlpm) \
	do { \
		_ctx.eop_ali_near_end_lec[_ch].nlpm = _nlpm; \
	} while (0)

extern int vin_coder_control(struct vinetic_context *ctx);

#define vin_coder_enable(_ctx) \
	({ \
		_ctx.eop_coder_control.en = VIN_EN; \
		int __res = vin_coder_control(&_ctx); \
		if (__res < 0) _ctx.eop_coder_control.en = VIN_DIS; \
		__res; \
	})

#define vin_coder_disable(_ctx) \
	({ \
		_ctx.eop_coder_control.en = VIN_DIS; \
		int __res = vin_coder_control(&_ctx); \
		if (__res < 0) _ctx.eop_coder_control.en = VIN_EN; \
		__res; \
	})

#define is_vin_coder_enabled(_ctx) \
	({ \
		int __res = 0; \
		if (_ctx.eop_coder_control.en == VIN_EN) \
			__res = 1; \
		__res; \
	})

#define is_vin_coder_used(_ctx) \
	({ \
		size_t __i; \
		int __res = 0; \
		for (__i=0; __i<4; __i++) \
		{ \
			if (_ctx.eop_coder_channel_speech_compression[__i].en == VIN_EN) { \
				__res = 1; \
				break; \
			} \
		} \
		__res; \
	})

extern int vin_coder_channel_speech_compression(struct vinetic_context *ctx, unsigned int ch);

#define vin_coder_channel_enable(_ctx, _ch) \
	({ \
		_ctx.eop_coder_channel_speech_compression[_ch].en = VIN_EN; \
		_ctx.eop_coder_channel_speech_compression[_ch].codnr = _ch; \
		int __res = vin_coder_channel_speech_compression(&_ctx, _ch); \
		if (__res < 0) _ctx.eop_coder_channel_speech_compression[_ch].en = VIN_DIS; \
		__res; \
	})

#define vin_coder_channel_disable(_ctx, _ch) \
	({ \
		_ctx.eop_coder_channel_speech_compression[_ch].en = VIN_DIS; \
		int __res = vin_coder_channel_speech_compression(&_ctx, _ch); \
		if (__res < 0) _ctx.eop_coder_channel_speech_compression[_ch].en = VIN_EN; \
		__res; \
	})

#define is_vin_coder_channel_enabled(_ctx, _ch) \
	({ \
		int __res = 0; \
		if (_ctx.eop_coder_channel_speech_compression[_ch].en == VIN_EN) \
			__res = 1; \
		__res; \
	})

#define vin_coder_channel_set_ns(_ctx, _ch, _ns) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].ns = _ns; \
	} while (0)

#define vin_coder_channel_set_hp(_ctx, _ch, _hp) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].hp = _hp; \
	} while (0)

#define vin_coder_channel_set_pf(_ctx, _ch, _pf) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].pf = _pf; \
	} while (0)

#define vin_coder_channel_set_cng(_ctx, _ch, _cng) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].cng = _cng; \
	} while (0)

#define vin_coder_channel_set_bfi(_ctx, _ch, _bfi) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].bfi = _bfi; \
	} while (0)

#define vin_coder_channel_set_dec(_ctx, _ch, _dec) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].dec = _dec; \
	} while (0)

#define vin_coder_channel_set_im(_ctx, _ch, _im) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].im = _im; \
	} while (0)

#define vin_coder_channel_set_pst(_ctx, _ch, _pst) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].pst = _pst; \
	} while (0)

#define vin_coder_channel_set_sic(_ctx, _ch, _sic) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].sic = _sic; \
	} while (0)

#define vin_coder_channel_set_pte(_ctx, _ch, _pte) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].pte = _pte; \
	} while (0)

#define vin_coder_channel_set_enc(_ctx, _ch, _enc) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].enc = _enc; \
	} while (0)

#define vin_coder_channel_set_gain1(_ctx, _ch, _gain1) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].gain1 = _gain1; \
	} while (0)

#define vin_coder_channel_set_gain2(_ctx, _ch, _gain2) \
	do { \
		_ctx.eop_coder_channel_speech_compression[_ch].gain1 = _gain2; \
	} while (0)

#define vin_coder_channel_set_input_ali(_ctx, _ch, _inp, _ali) \
	do { \
		if (_inp == 5) \
			_ctx.eop_coder_channel_speech_compression[_ch].i5 = VIN_SIG_ALM_OUT00 + _ali; \
		else if (_inp == 4) \
			_ctx.eop_coder_channel_speech_compression[_ch].i4 = VIN_SIG_ALM_OUT00 + _ali; \
		else if (_inp == 3) \
			_ctx.eop_coder_channel_speech_compression[_ch].i3 = VIN_SIG_ALM_OUT00 + _ali; \
		else if (_inp == 2) \
			_ctx.eop_coder_channel_speech_compression[_ch].i2 = VIN_SIG_ALM_OUT00 + _ali; \
		else \
			_ctx.eop_coder_channel_speech_compression[_ch].i1 = VIN_SIG_ALM_OUT00 + _ali; \
	} while (0)

extern int vin_coder_channel_configuration_rtp_support(struct vinetic_context *ctx, unsigned int ch);

#define vin_coder_channel_config_rtp(_ctx, _ch) \
	({ \
		int __res = vin_coder_channel_configuration_rtp_support(&_ctx, _ch); \
		__res; \
	})

#define vin_coder_channel_config_rtp_set_ssrc(_ctx, _ch, _ssrc) \
	do { \
		_ctx.eop_coder_channel_configuration_rtp_support[_ch].ssrc_hw = htons(_ssrc >> 16); \
		_ctx.eop_coder_channel_configuration_rtp_support[_ch].ssrc_lw = htons(_ssrc); \
	} while (0)

#define vin_coder_channel_config_rtp_set_seq_nr(_ctx, _ch, _seq_nr) \
	do { \
		_ctx.eop_coder_channel_configuration_rtp_support[_ch].seq_nr = htons(_seq_nr); \
	} while (0)

extern int vin_coder_channel_jb_statistic_reset(struct vinetic_context *ctx, unsigned int chan);

#define VIN_GAINDB_MIN -24.08
#define VIN_GAINDB_MAX 23.95

extern double vin_gainem_to_gaindb(u_int8_t em);
extern u_int8_t vin_gaindb_to_gainem(double g);

#endif //__LIBVINETIC_H__

/******************************************************************************/
/* end of libvinetic.h                                                        */
/******************************************************************************/
