/******************************************************************************/
/* libvinetic.h                                                               */
/******************************************************************************/

#ifndef __LIBVINETIC__H__
#define __LIBVINETIC_H__

#include <arpa/inet.h>
#include <sys/types.h>
#include <limits.h>
#include <math.h>

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

	char *message_stack_buf;
	size_t message_stack_len;
	char *message_stack_ptr;
	const char *message_stack_out;

	u_int16_t revision;

	size_t resources[8]; // shared coder, data pump, pcm, cpt3

	struct vin_eop_ali_control eop_ali_control;
	struct vin_eop_ali_channel eop_ali_channel[4];
	struct vin_eop_ali_near_end_lec eop_ali_near_end_lec[4];
	unsigned int ali_opmode[4];

	struct vin_eop_coder_control eop_coder_control;
	struct vin_eop_coder_channel_speech_compression eop_coder_channel_speech_compression[4];
	struct vin_eop_coder_channel_configuration_rtp_support eop_coder_channel_configuration_rtp_support[4];

	struct vin_eop_signaling_control eop_signaling_control;
	struct vin_eop_signaling_channel eop_signaling_channel[4];
	struct vin_eop_dtmf_receiver eop_dtmf_receiver[4];
	struct vin_eop_utg eop_utg[4];
	struct vin_eop_coder_configuration_rtp_support eop_coder_configuration_rtp_support;
	struct vin_eop_signaling_channel_configuration_rtp_support eop_signaling_channel_configuration_rtp_support[4];

	struct vin_eop_utg_coefficients eop_utg_coefficients[4];

	struct vin_eop_edsp_sw_version_register edsp_sw_version_register;

	// status
	struct vin_status_registers status;
	struct vin_status_registers status_old;
	struct vin_status_registers status_mask;

	// hook handlers
	struct vinetic_hook_handler {
		void *data;
		void (* handler)(void *data, int hook_state);
	} vin_hook_handler[4];
};

extern void vin_init(struct vinetic_context *ctx, const char *fmt, ...);
extern int vin_set_pram(struct vinetic_context *ctx, const char *fmt, ...);
extern int vin_set_dram(struct vinetic_context *ctx, const char *fmt, ...);
extern int vin_set_alm_dsp_ab(struct vinetic_context *ctx, const char *fmt, ...);
extern int vin_set_alm_dsp_cd(struct vinetic_context *ctx, const char *fmt, ...);
extern int vin_set_cram(struct vinetic_context *ctx, const char *fmt, ...);
extern int vin_open(struct vinetic_context *ctx);
extern void vin_close(struct vinetic_context *ctx);
extern int vin_reset(struct vinetic_context *ctx);
extern int vin_reset_rdyq(struct vinetic_context *ctx);
extern int vin_flush_mbox(struct vinetic_context *ctx);
extern int vin_is_not_ready(struct vinetic_context *ctx);
extern u_int16_t vin_read_dia(struct vinetic_context *ctx);

extern int vin_reset_status(struct vinetic_context *ctx);
extern ssize_t vin_get_status(struct vinetic_context *ctx);
extern ssize_t vin_set_status_mask(struct vinetic_context *ctx);
extern void vin_status_monitor(struct vinetic_context *ctx);

extern int vin_resync(struct vinetic_context *ctx);
extern int vin_cerr_acknowledge(struct vinetic_context *ctx);

extern int vin_poll_set(struct vinetic_context *ctx, int poll);

extern char *vin_get_dev_name(struct vinetic_context *ctx);
extern void vin_set_dev_name(struct vinetic_context *ctx, char *name);

extern void vin_message_stack_printf(struct vinetic_context *ctx, const char *format, ...);
extern const char *vin_message_stack_check_line(struct vinetic_context *ctx);
extern const char *vin_message_stack_get_line(struct vinetic_context *ctx);

#define VIN_REV_13 0x2442
#define VIN_REV_14 0x2484
extern char *vin_revision_str(struct vinetic_context *ctx);


extern int vin_read_fw_version(struct vinetic_context *ctx);

extern ssize_t vin_write(struct vinetic_context *ctx, size_t track_err, const void *buf, size_t count);

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
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = vin_poll_set(__context, 1); \
	__res; \
})

#define vin_poll_disable(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = vin_poll_set(__context, 0); \
	__res; \
})

#define vin_set_little_endian_mode(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = vin_set_endian_mode(__context, VIN_LITTLE_ENDIAN); \
	__res; \
})

#define vin_get_resource(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = -1; \
	if (!__context->resources[7]) { \
		__context->resources[7] = 1; \
		__res = 7; \
	} else if (!__context->resources[6]) { \
		__context->resources[6] = 1; \
		__res = 6; \
	} else if (!__context->resources[5]) { \
		__context->resources[5] = 1; \
		__res = 5; \
	} else if (!__context->resources[4]) { \
		__context->resources[4] = 1; \
		__res = 4; \
	} else if (!__context->resources[0]) { \
		__context->resources[0] = 1; \
		__res = 0; \
	} else if (!__context->resources[1]) { \
		__context->resources[1] = 1; \
		__res = 1; \
	} else if (!__context->resources[2]) { \
		__context->resources[2] = 1; \
		__res = 2; \
	} else if (!__context->resources[3]) { \
		__context->resources[3] = 1; \
		__res = 3; \
	} \
	__res; \
})

#define vin_put_resource(_ctx, _nr) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->resources[_nr] = 0; \
} while (0)

extern int vin_ali_control(struct vinetic_context *ctx);

#define vin_ali_enable(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_control.en = VIN_EN; \
	int __res = vin_ali_control(_ctx); \
	if (__res < 0) { \
		__context->eop_ali_control.en = VIN_DIS; \
	} \
	__res; \
})

#define vin_ali_disable(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_control.en = VIN_DIS; \
	int __res = vin_ali_control(__context); \
	__res; \
})

#define is_vin_ali_enabled(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if (__context->eop_ali_control.en == VIN_EN) { \
		__res = 1; \
	} \
	__res; \
})

#define is_vin_ali_used(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	size_t __i; \
	int __res = 0; \
	for (__i = 0; __i < 4; __i++) { \
		if (__context->eop_ali_channel[__i].en == VIN_EN) { \
			__res = 1; \
			break; \
		} \
	} \
	__res; \
})

extern int vin_ali_channel(struct vinetic_context *ctx, unsigned int ch);

#define vin_ali_channel_enable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_channel[_ch].en = VIN_EN; \
	int __res = vin_ali_channel(_ctx, _ch); \
	if (__res < 0) { \
		__context->eop_ali_channel[_ch].en = VIN_DIS; \
	} \
	__res; \
})

#define vin_ali_channel_disable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_channel[_ch].en = VIN_DIS; \
	int __res = vin_ali_channel(_ctx, _ch); \
	__res; \
})

#define vin_ali_channel_reset(_ctx, _ch) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if ((_ch >= 0) && (_ch <= 3)) { \
		__context->eop_ali_channel[_ch].en = VIN_DIS; \
	} \
} while (0)

#define is_vin_ali_channel_enabled(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if ((_ch >= 0) && (__context->eop_ali_channel[_ch].en == VIN_EN)) { \
		__res = 1; \
	} \
	__res; \
})

#define vin_ali_channel_set_gainr(_ctx, _ch, _gainr) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_channel[_ch].gainr = _gainr; \
} while (0)

#define vin_ali_channel_set_gainx(_ctx, _ch, _gainx) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_channel[_ch].gainx = _gainx; \
} while (0)

#define vin_ali_channel_set_input_coder(_ctx, _ch, _inp, _cod) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_ali_channel[_ch].i5 = VIN_SIG_COD_OUT00 + _cod; \
	} else if (_inp == 4) { \
		__context->eop_ali_channel[_ch].i4 = VIN_SIG_COD_OUT00 + _cod; \
	} else if (_inp == 3) { \
		__context->eop_ali_channel[_ch].i3 = VIN_SIG_COD_OUT00 + _cod; \
	} else if (_inp == 2) { \
		__context->eop_ali_channel[_ch].i2 = VIN_SIG_COD_OUT00 + _cod; \
	} else { \
		__context->eop_ali_channel[_ch].i1 = VIN_SIG_COD_OUT00 + _cod; \
	} \
} while (0)

#define vin_ali_channel_set_input_sig_a(_ctx, _ch, _inp, _sig_a) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_ali_channel[_ch].i5 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} else if (_inp == 4) { \
		__context->eop_ali_channel[_ch].i4 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} else if (_inp == 3) { \
		__context->eop_ali_channel[_ch].i3 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} else if (_inp == 2) { \
		__context->eop_ali_channel[_ch].i2 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} else { \
		__context->eop_ali_channel[_ch].i1 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} \
} while (0)

#define vin_ali_channel_set_input_sig_b(_ctx, _ch, _inp, _sig_b) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_ali_channel[_ch].i5 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} else if (_inp == 4) { \
		__context->eop_ali_channel[_ch].i4 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} else if (_inp == 3) { \
		__context->eop_ali_channel[_ch].i3 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} else if (_inp == 2) { \
		__context->eop_ali_channel[_ch].i2 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} else { \
		__context->eop_ali_channel[_ch].i1 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} \
} while (0)

extern int vin_ali_near_end_lec(struct vinetic_context *ctx, unsigned int ch);

#define vin_ali_near_end_lec_enable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_near_end_lec[_ch].en = VIN_EN; \
	__context->eop_ali_near_end_lec[_ch].lecnr = _ch; \
	int __res = vin_ali_near_end_lec(_ctx, _ch); \
	if (__res < 0) { \
		__context->eop_ali_near_end_lec[_ch].en = VIN_DIS; \
	} \
	__res; \
})

#define vin_ali_near_end_lec_disable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_near_end_lec[_ch].en = VIN_DIS; \
	int __res = vin_ali_near_end_lec(_ctx, _ch); \
	__res; \
})

#define is_vin_ali_near_end_lec_enabled(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if ((_ch >= 0) && (__context->eop_ali_near_end_lec[_ch].en == VIN_EN)) { \
		__res = 1; \
	} \
	__res; \
})

#define vin_ali_near_end_lec_set_dtm(_ctx, _ch, _dtm) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_near_end_lec[_ch].dtm = _dtm; \
} while (0)

#define vin_ali_near_end_lec_set_oldc(_ctx, _ch, _oldc) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_near_end_lec[_ch].oldc = _oldc; \
} while (0)

#define vin_ali_near_end_lec_set_as(_ctx, _ch, _as) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_near_end_lec[_ch].as = _as; \
} while (0)

#define vin_ali_near_end_lec_set_nlp(_ctx, _ch, _nlp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_near_end_lec[_ch].nlp = _nlp; \
} while (0)

#define vin_ali_near_end_lec_set_nlpm(_ctx, _ch, _nlpm) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_ali_near_end_lec[_ch].nlpm = _nlpm; \
} while (0)

extern int vin_signaling_control(struct vinetic_context *ctx);

#define vin_signaling_enable(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_signaling_control.en = VIN_EN; \
	int __res = vin_signaling_control(__context); \
	if (__res < 0) { \
		__context->eop_signaling_control.en = VIN_DIS; \
	} \
	__res; \
})

#define vin_signaling_disable(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_signaling_control.en = VIN_DIS; \
	int __res = vin_signaling_control(__context); \
	__res; \
})

#define is_vin_signaling_enabled(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if (__context->eop_signaling_control.en == VIN_EN) { \
		__res = 1; \
	} \
	__res; \
})

#define is_vin_signaling_used(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	size_t __i; \
	int __res = 0; \
	for (__i = 0; __i < 4; __i++) { \
		if (__context->eop_signaling_channel[__i].en == VIN_EN) { \
			__res = 1; \
			break; \
		} \
	} \
	__res; \
})

#define get_vin_signaling_channel(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	size_t __i; \
	int __res = -1; \
	for (__i = 0; __i < 4; __i++) { \
		if (__context->eop_signaling_channel[__i].en == VIN_DIS) { \
			__res = (int)__i; \
			break; \
		} \
	} \
	__res; \
})

extern int vin_signaling_channel(struct vinetic_context *ctx, unsigned int ch);

#define vin_signaling_channel_enable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_signaling_channel[_ch].en = VIN_EN; \
	int __res = vin_signaling_channel(_ctx, _ch); \
	if (__res < 0) __context->eop_signaling_channel[_ch].en = VIN_DIS; \
	__res; \
})

#define vin_signaling_channel_disable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_signaling_channel[_ch].en = VIN_DIS; \
	int __res = vin_signaling_channel(_ctx, _ch); \
	__res; \
})

#define vin_signaling_channel_reset(_ctx, _ch) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if ((_ch >= 0) && (_ch <= 3)) { \
		__context->eop_signaling_channel[_ch].en = VIN_DIS; \
	} \
} while (0)

#define is_vin_signaling_channel_enabled(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if ((_ch >= 0) && (__context->eop_signaling_channel[_ch].en == VIN_EN)) \
		__res = 1; \
	__res; \
})

#define vin_signaling_channel_set_input_null(_ctx, _ch, _inp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 2) { \
		__context->eop_signaling_channel[_ch].i2 = VIN_SIG_NULL; \
	} else { \
		__context->eop_signaling_channel[_ch].i1 = VIN_SIG_NULL; \
	} \
} while (0)

#define vin_signaling_channel_set_input_pcm(_ctx, _ch, _inp, _pcm) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 2) { \
		__context->eop_signaling_channel[_ch].i2 = VIN_SIG_PCM_OUT00 + _pcm; \
	} else { \
		__context->eop_signaling_channel[_ch].i1 = VIN_SIG_PCM_OUT00 + _pcm; \
	} \
} while (0)

#define vin_signaling_channel_set_input_ali(_ctx, _ch, _inp, _ali) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 2) { \
		__context->eop_signaling_channel[_ch].i2 = VIN_SIG_ALM_OUT00 + _ali; \
	} else { \
		__context->eop_signaling_channel[_ch].i1 = VIN_SIG_ALM_OUT00 + _ali; \
	} \
} while (0)

#define vin_signaling_channel_set_input_coder(_ctx, _ch, _inp, _cod) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 2) { \
		__context->eop_signaling_channel[_ch].i2 = VIN_SIG_COD_OUT00 + _cod; \
	} else { \
		__context->eop_signaling_channel[_ch].i1 = VIN_SIG_COD_OUT00 + _cod; \
	} \
} while (0)

extern int vin_dtmf_receiver(struct vinetic_context *ctx, unsigned int ch);

#define vin_dtmf_receiver_enable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_dtmf_receiver[_ch].en = VIN_EN; \
	__context->eop_dtmf_receiver[_ch].dtrnr = _ch; \
	int __res = vin_dtmf_receiver(_ctx, _ch); \
	if (__res < 0) { \
		__context->eop_dtmf_receiver[_ch].en = VIN_DIS; \
	} \
	__res; \
})

#define vin_dtmf_receiver_disable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_dtmf_receiver[_ch].en = VIN_DIS; \
	int __res = vin_dtmf_receiver(_ctx, _ch); \
	__res; \
})

#define is_vin_dtmf_receiver_enabled(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if ((_ch >= 0) && (__context->eop_dtmf_receiver[_ch].en == VIN_EN)) { \
		__res = 1; \
	} \
	__res; \
})

#define vin_dtmf_receiver_set_as(_ctx, _ch, _as) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_dtmf_receiver[_ch].as = _as; \
} while (0)

#define vin_dtmf_receiver_set_is(_ctx, _ch, _is) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_dtmf_receiver[_ch].is = _is; \
} while (0)

#define vin_dtmf_receiver_set_et(_ctx, _ch, _et) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_dtmf_receiver[_ch].et = _et; \
} while (0)

extern int vin_utg(struct vinetic_context *ctx, unsigned int ch);

#define vin_utg_enable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg[_ch].en = VIN_EN; \
	__context->eop_utg[_ch].utgnr = _ch; \
	int __res = vin_utg(_ctx, _ch); \
	if (__res < 0) { \
		__context->eop_utg[_ch].en = VIN_DIS; \
	} \
	__res; \
})

#define vin_utg_disable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg[_ch].en = VIN_DIS; \
	int __res = vin_utg(_ctx, _ch); \
	__res; \
})

#define is_vin_utg_enabled(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if ((_ch >= 0) && (__context->eop_utg[_ch].en == VIN_EN)) { \
		__res = 1; \
	} \
	__res; \
})

#define vin_utg_set_add_b(_ctx, _ch, _add_b) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg[_ch].add_b = _add_b; \
} while (0)

#define vin_utg_set_add_a(_ctx, _ch, _add_a) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg[_ch].add_a = _add_a; \
} while (0)

#define vin_utg_set_log(_ctx, _ch, _log) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg[_ch].log = _log; \
} while (0)

#define vin_utg_set_sq(_ctx, _ch, _sq) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg[_ch].sq = _sq; \
} while (0)

#define vin_utg_set_sm(_ctx, _ch, _sm) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg[_ch].sm = _sm; \
} while (0)

extern int vin_signaling_channel_configuration_rtp_support(struct vinetic_context *ctx, unsigned int ch);

#define vin_signaling_channel_config_rtp(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = vin_signaling_channel_configuration_rtp_support(__context, _ch); \
	__res; \
})

#define vin_signaling_channel_config_rtp_set_ssrc(_ctx, _ch, _ssrc) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_signaling_channel_configuration_rtp_support[_ch].ssrc_hw = htons(_ssrc >> 16); \
	__context->eop_signaling_channel_configuration_rtp_support[_ch].ssrc_lw = htons(_ssrc); \
} while (0)

#define vin_signaling_channel_config_rtp_set_evt_pt(_ctx, _ch, _evt_pt) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_signaling_channel_configuration_rtp_support[_ch].evt_pt = _evt_pt; \
} while (0)

extern int vin_coder_control(struct vinetic_context *ctx);

#define vin_coder_enable(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_control.en = VIN_EN; \
	int __res = vin_coder_control(__context); \
	if (__res < 0) { \
		__context->eop_coder_control.en = VIN_DIS; \
	} \
	__res; \
})

#define vin_coder_disable(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_control.en = VIN_DIS; \
	int __res = vin_coder_control(__context); \
	__res; \
})

#define is_vin_coder_enabled(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if (__context->eop_coder_control.en == VIN_EN) { \
			__res = 1; \
	} \
	__res; \
})

#define is_vin_coder_used(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	size_t __i; \
	int __res = 0; \
	for (__i = 0; __i < 4; __i++) { \
		if (__context->eop_coder_channel_speech_compression[__i].en == VIN_EN) { \
			__res = 1; \
			break; \
		} \
	} \
	__res; \
})

extern int vin_coder_channel_speech_compression(struct vinetic_context *ctx, unsigned int ch);

#define vin_coder_channel_enable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res; \
	__context->eop_coder_channel_speech_compression[_ch].codnr = vin_get_resource(_ctx); \
	if (__context->eop_coder_channel_speech_compression[_ch].codnr < 0) { \
		vin_message_stack_printf(_ctx, "coder resources exhausted"); \
		__res = -1; \
	} else { \
		__context->eop_coder_channel_speech_compression[_ch].en = VIN_EN; \
		__res = vin_coder_channel_speech_compression(_ctx, _ch); \
		if (__res < 0) { \
			vin_put_resource(_ctx, __context->eop_coder_channel_speech_compression[_ch].codnr); \
			__context->eop_coder_channel_speech_compression[_ch].en = VIN_DIS; \
		} \
	} \
	__res; \
})

#define vin_coder_channel_disable(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	vin_put_resource(_ctx, __context->eop_coder_channel_speech_compression[_ch].codnr); \
	__context->eop_coder_channel_speech_compression[_ch].en = VIN_DIS; \
	int __res = vin_coder_channel_speech_compression(_ctx, _ch); \
	__res; \
})

#define vin_coder_channel_reset(_ctx, _ch) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if ((_ch >= 0) && (_ch <= 3)) { \
		vin_put_resource(_ctx, __context->eop_coder_channel_speech_compression[_ch].codnr); \
		__context->eop_coder_channel_speech_compression[_ch].en = VIN_DIS; \
	} \
} while (0)

#define is_vin_coder_channel_enabled(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = 0; \
	if ((_ch >= 0) && (__context->eop_coder_channel_speech_compression[_ch].en == VIN_EN)) { \
		__res = 1; \
	} \
	__res; \
})

#define vin_coder_channel_set_ns(_ctx, _ch, _ns) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].ns = _ns; \
} while (0)

#define vin_coder_channel_set_hp(_ctx, _ch, _hp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].hp = _hp; \
} while (0)

#define vin_coder_channel_set_pf(_ctx, _ch, _pf) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].pf = _pf; \
} while (0)

#define vin_coder_channel_set_cng(_ctx, _ch, _cng) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].cng = _cng; \
} while (0)

#define vin_coder_channel_set_bfi(_ctx, _ch, _bfi) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].bfi = _bfi; \
} while (0)

#define vin_coder_channel_set_dec(_ctx, _ch, _dec) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].dec = _dec; \
} while (0)

#define vin_coder_channel_set_im(_ctx, _ch, _im) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].im = _im; \
} while (0)

#define vin_coder_channel_set_pst(_ctx, _ch, _pst) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].pst = _pst; \
} while (0)

#define vin_coder_channel_set_sic(_ctx, _ch, _sic) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].sic = _sic; \
} while (0)

#define vin_coder_channel_set_pte(_ctx, _ch, _pte) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].pte = _pte; \
} while (0)

#define vin_coder_channel_set_enc(_ctx, _ch, _enc) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].enc = _enc; \
} while (0)

#define vin_coder_channel_set_gain1(_ctx, _ch, _gain1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].gain1 = _gain1; \
} while (0)

#define vin_coder_channel_set_gain2(_ctx, _ch, _gain2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_speech_compression[_ch].gain2 = _gain2; \
} while (0)

#define vin_coder_channel_set_input_null(_ctx, _ch, _inp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_coder_channel_speech_compression[_ch].i5 = VIN_SIG_NULL; \
	} else if (_inp == 4) {\
		__context->eop_coder_channel_speech_compression[_ch].i4 = VIN_SIG_NULL; \
	} else if (_inp == 3) { \
		__context->eop_coder_channel_speech_compression[_ch].i3 = VIN_SIG_NULL; \
	} else if (_inp == 2) { \
		__context->eop_coder_channel_speech_compression[_ch].i2 = VIN_SIG_NULL; \
	} else { \
		__context->eop_coder_channel_speech_compression[_ch].i1 = VIN_SIG_NULL; \
	} \
} while (0)

#define vin_coder_channel_set_input_pcm(_ctx, _ch, _inp, _pcm) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_coder_channel_speech_compression[_ch].i5 = VIN_SIG_PCM_OUT00 + _pcm; \
	} else if (_inp == 4) {\
		__context->eop_coder_channel_speech_compression[_ch].i4 = VIN_SIG_PCM_OUT00 + _pcm; \
	} else if (_inp == 3) { \
		__context->eop_coder_channel_speech_compression[_ch].i3 = VIN_SIG_PCM_OUT00 + _pcm; \
	} else if (_inp == 2) { \
		__context->eop_coder_channel_speech_compression[_ch].i2 = VIN_SIG_PCM_OUT00 + _pcm; \
	} else { \
		__context->eop_coder_channel_speech_compression[_ch].i1 = VIN_SIG_PCM_OUT00 + _pcm; \
	} \
} while (0)

#define vin_coder_channel_set_input_ali(_ctx, _ch, _inp, _ali) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_coder_channel_speech_compression[_ch].i5 = VIN_SIG_ALM_OUT00 + _ali; \
	} else if (_inp == 4) {\
		__context->eop_coder_channel_speech_compression[_ch].i4 = VIN_SIG_ALM_OUT00 + _ali; \
	} else if (_inp == 3) { \
		__context->eop_coder_channel_speech_compression[_ch].i3 = VIN_SIG_ALM_OUT00 + _ali; \
	} else if (_inp == 2) { \
		__context->eop_coder_channel_speech_compression[_ch].i2 = VIN_SIG_ALM_OUT00 + _ali; \
	} else { \
		__context->eop_coder_channel_speech_compression[_ch].i1 = VIN_SIG_ALM_OUT00 + _ali; \
	} \
} while (0)

#define vin_coder_channel_set_input_coder(_ctx, _ch, _inp, _cod) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_coder_channel_speech_compression[_ch].i5 = VIN_SIG_COD_OUT00 + _cod; \
	} else if (_inp == 4) {\
		__context->eop_coder_channel_speech_compression[_ch].i4 = VIN_SIG_COD_OUT00 + _cod; \
	} else if (_inp == 3) { \
		__context->eop_coder_channel_speech_compression[_ch].i3 = VIN_SIG_COD_OUT00 + _cod; \
	} else if (_inp == 2) { \
		__context->eop_coder_channel_speech_compression[_ch].i2 = VIN_SIG_COD_OUT00 + _cod; \
	} else { \
		__context->eop_coder_channel_speech_compression[_ch].i1 = VIN_SIG_COD_OUT00 + _cod; \
	} \
} while (0)

#define vin_coder_channel_set_input_sig_a(_ctx, _ch, _inp, _sig_a) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_coder_channel_speech_compression[_ch].i5 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} else if (_inp == 4) { \
		__context->eop_coder_channel_speech_compression[_ch].i4 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} else if (_inp == 3) { \
		__context->eop_coder_channel_speech_compression[_ch].i3 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} else if (_inp == 2) { \
		__context->eop_coder_channel_speech_compression[_ch].i2 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} else { \
		__context->eop_coder_channel_speech_compression[_ch].i1 = VIN_SIG_SIG_OUTA0 + _sig_a * 2; \
	} \
} while (0)

#define vin_coder_channel_set_input_sig_b(_ctx, _ch, _inp, _sig_b) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_inp == 5) { \
		__context->eop_coder_channel_speech_compression[_ch].i5 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} else if (_inp == 4) { \
		__context->eop_coder_channel_speech_compression[_ch].i4 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} else if (_inp == 3) { \
		__context->eop_coder_channel_speech_compression[_ch].i3 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} else if (_inp == 2) { \
		__context->eop_coder_channel_speech_compression[_ch].i2 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} else { \
		__context->eop_coder_channel_speech_compression[_ch].i1 = VIN_SIG_SIG_OUTB0 + _sig_b * 2; \
	} \
} while (0)

extern int vin_coder_configuration_rtp_support(struct vinetic_context *ctx);

#define vin_coder_config_rtp(_ctx) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = vin_coder_configuration_rtp_support(__context); \
	__res; \
})

#define vin_coder_config_rtp_set_timestamp(_ctx, _timestamp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_configuration_rtp_support.time_stamp_hw = htons(_timestamp >> 16); \
	__context->eop_coder_configuration_rtp_support.time_stamp_lw = htons(_timestamp); \
} while (0)

extern int vin_coder_channel_configuration_rtp_support(struct vinetic_context *ctx, unsigned int ch);

#define vin_coder_channel_config_rtp(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res = vin_coder_channel_configuration_rtp_support(__context, _ch); \
	__res; \
})

#define vin_coder_channel_config_rtp_set_ssrc(_ctx, _ch, _ssrc) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_configuration_rtp_support[_ch].ssrc_hw = htons(_ssrc >> 16); \
	__context->eop_coder_channel_configuration_rtp_support[_ch].ssrc_lw = htons(_ssrc); \
} while (0)

#define vin_coder_channel_config_rtp_set_seq_nr(_ctx, _ch, _seq_nr) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_coder_channel_configuration_rtp_support[_ch].seq_nr = htons(_seq_nr); \
} while (0)

extern int vin_coder_channel_jb_statistic_reset(struct vinetic_context *ctx, unsigned int chan);

extern int vin_utg_coefficients(struct vinetic_context *ctx, unsigned int rw, unsigned int ch);

extern int vin_utg_set_asterisk_tone(struct vinetic_context *ctx, unsigned int ch, const char *tone);

#define vin_utg_coefficients_write(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res; \
	__res = vin_utg_coefficients(__context, VIN_WRITE, _ch); \
	__res; \
})

#define vin_utg_coefficients_read(_ctx, _ch) \
({ \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	int __res; \
	__res = vin_utg_coefficients(__context, VIN_READ, _ch); \
	__res; \
})

#define vin_utg_coefficients_set_fd_in_att(_ctx, _ch, _fd_in_att) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].fd_in_att = (u_int16_t)(32767.f * powf(10.f, (-_fd_in_att / 20.f))); \
} while (0)

#define vin_utg_coefficients_set_fd_in_sp(_ctx, _ch, _fd_in_sp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].fd_in_sp = (u_int16_t)(32768.f * _fd_in_sp); \
} while (0)

#define vin_utg_coefficients_set_fd_in_sp_log(_ctx, _ch, _fd_in_sp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].fd_in_sp = (u_int16_t)(256.f * powf(10.f, (_fd_in_sp / 20.f))); \
} while (0)

#define vin_utg_coefficients_set_fd_in_tim(_ctx, _ch, _fd_in_tim) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].fd_in_tim = _fd_in_tim * 8; \
} while (0)

#define vin_utg_coefficients_set_fd_ot_sp(_ctx, _ch, _fd_ot_sp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].fd_ot_sp = (u_int16_t)(32768.f * _fd_ot_sp); \
} while (0)

#define vin_utg_coefficients_set_fd_ot_sp_log(_ctx, _ch, _fd_ot_sp) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].fd_ot_sp = (u_int16_t)(256.f * powf(10.f, (_fd_ot_sp / 20.f))); \
} while (0)

#define vin_utg_coefficients_set_fd_ot_tim(_ctx, _ch, _fd_ot_tim) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].fd_ot_tim = _fd_ot_tim * 8; \
} while (0)

#define vin_utg_coefficients_set_mod_12(_ctx, _ch, _mod_12) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].mod_12 = (int8_t)(128.f * -_mod_12); \
} while (0)

#define vin_utg_coefficients_set_f1(_ctx, _ch, _f1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].f1 = (_f1 * 8192) / 1000; \
} while (0)

#define vin_utg_coefficients_set_f2(_ctx, _ch, _f2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].f2 = (_f2 * 8192) / 1000; \
} while (0)

#define vin_utg_coefficients_set_f3(_ctx, _ch, _f3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].f3 = (_f3 * 8192) / 1000; \
} while (0)

#define vin_utg_coefficients_set_f4(_ctx, _ch, _f4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].f4 = (_f4 * 8192) / 1000; \
} while (0)

#define vin_utg_coefficients_set_lev_1(_ctx, _ch, _lev_1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].lev_1 = (u_int8_t)(256.f * powf(10.f, (_lev_1 / 20.f))); \
} while (0)

#define vin_utg_coefficients_set_lev_2(_ctx, _ch, _lev_2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].lev_2 = (u_int8_t)(256.f * powf(10.f, (_lev_2 / 20.f))); \
} while (0)

#define vin_utg_coefficients_set_lev_3(_ctx, _ch, _lev_3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].lev_3 = (u_int8_t)(256.f * powf(10.f, (_lev_3 / 20.f))); \
} while (0)

#define vin_utg_coefficients_set_lev_4(_ctx, _ch, _lev_4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].lev_4 = (u_int8_t)(256.f * powf(10.f, (_lev_4 / 20.f))); \
} while (0)

#define vin_utg_coefficients_set_t_1(_ctx, _ch, _t_1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_t_1 == (int)-1) { \
		__context->eop_utg_coefficients[_ch].t_1 = 0xFFFF; \
	} else { \
		__context->eop_utg_coefficients[_ch].t_1 = _t_1 * 2; \
	} \
} while (0)

#define vin_utg_coefficients_set_msk_1_nxt(_ctx, _ch, _nxt) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.nxt = _nxt; \
} while (0)

#define vin_utg_coefficients_set_msk_1_fi(_ctx, _ch, _fi) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.fi = _fi; \
} while (0)

#define vin_utg_coefficients_set_msk_1_fo(_ctx, _ch, _fo) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.fo = _fo; \
} while (0)

#define vin_utg_coefficients_set_msk_1_f1(_ctx, _ch, _f1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.f1 = _f1; \
} while (0)

#define vin_utg_coefficients_set_msk_1_f2(_ctx, _ch, _f2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.f2 = _f2; \
} while (0)

#define vin_utg_coefficients_set_msk_1_f3(_ctx, _ch, _f3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.f3 = _f3; \
} while (0)

#define vin_utg_coefficients_set_msk_1_f4(_ctx, _ch, _f4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.f4 = _f4; \
} while (0)

#define vin_utg_coefficients_set_msk_1_m12(_ctx, _ch, _m12) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.m12 = _m12; \
} while (0)

#define vin_utg_coefficients_set_msk_1_rep(_ctx, _ch, _rep) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.rep = _rep; \
} while (0)

#define vin_utg_coefficients_set_msk_1_sa(_ctx, _ch, _sa) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_1.sa = _sa; \
} while (0)

#define vin_utg_coefficients_set_t_2(_ctx, _ch, _t_2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_t_2 == (int)-1) { \
		__context->eop_utg_coefficients[_ch].t_2 = 0xFFFF; \
	} else { \
		__context->eop_utg_coefficients[_ch].t_2 = _t_2 * 2; \
	} \
} while (0)

#define vin_utg_coefficients_set_msk_2_nxt(_ctx, _ch, _nxt) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.nxt = _nxt; \
} while (0)

#define vin_utg_coefficients_set_msk_2_fi(_ctx, _ch, _fi) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.fi = _fi; \
} while (0)

#define vin_utg_coefficients_set_msk_2_fo(_ctx, _ch, _fo) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.fo = _fo; \
} while (0)

#define vin_utg_coefficients_set_msk_2_f1(_ctx, _ch, _f1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.f1 = _f1; \
} while (0)

#define vin_utg_coefficients_set_msk_2_f2(_ctx, _ch, _f2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.f2 = _f2; \
} while (0)

#define vin_utg_coefficients_set_msk_2_f3(_ctx, _ch, _f3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.f3 = _f3; \
} while (0)

#define vin_utg_coefficients_set_msk_2_f4(_ctx, _ch, _f4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.f4 = _f4; \
} while (0)

#define vin_utg_coefficients_set_msk_2_m12(_ctx, _ch, _m12) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.m12 = _m12; \
} while (0)

#define vin_utg_coefficients_set_msk_2_rep(_ctx, _ch, _rep) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.rep = _rep; \
} while (0)

#define vin_utg_coefficients_set_msk_2_sa(_ctx, _ch, _sa) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_2.sa = _sa; \
} while (0)

#define vin_utg_coefficients_set_t_3(_ctx, _ch, _t_3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_t_3 == (int)-1) { \
		__context->eop_utg_coefficients[_ch].t_3 = 0xFFFF; \
	} else { \
		__context->eop_utg_coefficients[_ch].t_3 = _t_3 * 2; \
	} \
} while (0)

#define vin_utg_coefficients_set_msk_3_nxt(_ctx, _ch, _nxt) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.nxt = _nxt; \
} while (0)

#define vin_utg_coefficients_set_msk_3_fi(_ctx, _ch, _fi) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.fi = _fi; \
} while (0)

#define vin_utg_coefficients_set_msk_3_fo(_ctx, _ch, _fo) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.fo = _fo; \
} while (0)

#define vin_utg_coefficients_set_msk_3_f1(_ctx, _ch, _f1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.f1 = _f1; \
} while (0)

#define vin_utg_coefficients_set_msk_3_f2(_ctx, _ch, _f2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.f2 = _f2; \
} while (0)

#define vin_utg_coefficients_set_msk_3_f3(_ctx, _ch, _f3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.f3 = _f3; \
} while (0)

#define vin_utg_coefficients_set_msk_3_f4(_ctx, _ch, _f4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.f4 = _f4; \
} while (0)

#define vin_utg_coefficients_set_msk_3_m12(_ctx, _ch, _m12) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.m12 = _m12; \
} while (0)

#define vin_utg_coefficients_set_msk_3_rep(_ctx, _ch, _rep) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.rep = _rep; \
} while (0)

#define vin_utg_coefficients_set_msk_3_sa(_ctx, _ch, _sa) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_3.sa = _sa; \
} while (0)

#define vin_utg_coefficients_set_t_4(_ctx, _ch, _t_4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_t_4 == (int)-1) { \
		__context->eop_utg_coefficients[_ch].t_4 = 0xFFFF; \
	} else { \
		__context->eop_utg_coefficients[_ch].t_4 = _t_4 * 2; \
	} \
} while (0)

#define vin_utg_coefficients_set_msk_4_nxt(_ctx, _ch, _nxt) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.nxt = _nxt; \
} while (0)

#define vin_utg_coefficients_set_msk_4_fi(_ctx, _ch, _fi) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.fi = _fi; \
} while (0)

#define vin_utg_coefficients_set_msk_4_fo(_ctx, _ch, _fo) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.fo = _fo; \
} while (0)

#define vin_utg_coefficients_set_msk_4_f1(_ctx, _ch, _f1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.f1 = _f1; \
} while (0)

#define vin_utg_coefficients_set_msk_4_f2(_ctx, _ch, _f2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.f2 = _f2; \
} while (0)

#define vin_utg_coefficients_set_msk_4_f3(_ctx, _ch, _f3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.f3 = _f3; \
} while (0)

#define vin_utg_coefficients_set_msk_4_f4(_ctx, _ch, _f4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.f4 = _f4; \
} while (0)

#define vin_utg_coefficients_set_msk_4_m12(_ctx, _ch, _m12) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.m12 = _m12; \
} while (0)

#define vin_utg_coefficients_set_msk_4_rep(_ctx, _ch, _rep) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.rep = _rep; \
} while (0)

#define vin_utg_coefficients_set_msk_4_sa(_ctx, _ch, _sa) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_4.sa = _sa; \
} while (0)

#define vin_utg_coefficients_set_t_5(_ctx, _ch, _t_5) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_t_5 == (int)-1) { \
		__context->eop_utg_coefficients[_ch].t_5 = 0xFFFF; \
	} else { \
		__context->eop_utg_coefficients[_ch].t_5 = _t_5 * 2; \
	} \
} while (0)

#define vin_utg_coefficients_set_msk_5_nxt(_ctx, _ch, _nxt) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.nxt = _nxt; \
} while (0)

#define vin_utg_coefficients_set_msk_5_fi(_ctx, _ch, _fi) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.fi = _fi; \
} while (0)

#define vin_utg_coefficients_set_msk_5_fo(_ctx, _ch, _fo) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.fo = _fo; \
} while (0)

#define vin_utg_coefficients_set_msk_5_f1(_ctx, _ch, _f1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.f1 = _f1; \
} while (0)

#define vin_utg_coefficients_set_msk_5_f2(_ctx, _ch, _f2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.f2 = _f2; \
} while (0)

#define vin_utg_coefficients_set_msk_5_f3(_ctx, _ch, _f3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.f3 = _f3; \
} while (0)

#define vin_utg_coefficients_set_msk_5_f4(_ctx, _ch, _f4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.f4 = _f4; \
} while (0)

#define vin_utg_coefficients_set_msk_5_m12(_ctx, _ch, _m12) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.m12 = _m12; \
} while (0)

#define vin_utg_coefficients_set_msk_5_rep(_ctx, _ch, _rep) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.rep = _rep; \
} while (0)

#define vin_utg_coefficients_set_msk_5_sa(_ctx, _ch, _sa) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_5.sa = _sa; \
} while (0)

#define vin_utg_coefficients_set_t_6(_ctx, _ch, _t_6) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	if (_t_6 == (int)-1) { \
		__context->eop_utg_coefficients[_ch].t_6 = 0xFFFF; \
	} else { \
		__context->eop_utg_coefficients[_ch].t_6 = _t_6 * 2; \
	} \
} while (0)

#define vin_utg_coefficients_set_msk_6_nxt(_ctx, _ch, _nxt) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.nxt = _nxt; \
} while (0)

#define vin_utg_coefficients_set_msk_6_fi(_ctx, _ch, _fi) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.fi = _fi; \
} while (0)

#define vin_utg_coefficients_set_msk_6_fo(_ctx, _ch, _fo) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.fo = _fo; \
} while (0)

#define vin_utg_coefficients_set_msk_6_f1(_ctx, _ch, _f1) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.f1 = _f1; \
} while (0)

#define vin_utg_coefficients_set_msk_6_f2(_ctx, _ch, _f2) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.f2 = _f2; \
} while (0)

#define vin_utg_coefficients_set_msk_6_f3(_ctx, _ch, _f3) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.f3 = _f3; \
} while (0)

#define vin_utg_coefficients_set_msk_6_f4(_ctx, _ch, _f4) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.f4 = _f4; \
} while (0)

#define vin_utg_coefficients_set_msk_6_m12(_ctx, _ch, _m12) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.m12 = _m12; \
} while (0)

#define vin_utg_coefficients_set_msk_6_rep(_ctx, _ch, _rep) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.rep = _rep; \
} while (0)

#define vin_utg_coefficients_set_msk_6_sa(_ctx, _ch, _sa) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].msk_6.sa = _sa; \
} while (0)

#define vin_utg_coefficients_set_go_add_a(_ctx, _ch, _go_add_a) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].go_add_a = (u_int16_t)(32768.f * powf(10.f, (_go_add_a / 20.f))); \
} while (0)

#define vin_utg_coefficients_set_go_add_b(_ctx, _ch, _go_add_b) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->eop_utg_coefficients[_ch].go_add_b = (u_int16_t)(32768.f * powf(10.f, (_go_add_b / 20.f))); \
} while (0)

#define VIN_GAINDB_MIN -24.08
#define VIN_GAINDB_MAX 23.95

extern double vin_gainem_to_gaindb(u_int8_t em);
extern u_int8_t vin_gaindb_to_gainem(double g);

#define vin_set_hook_handler(_ctx, _ch, _handler, _data) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->vin_hook_handler[_ch].handler = _handler; \
	__context->vin_hook_handler[_ch].data = _data; \
	if (_ch == 3) { \
		__context->status_mask.sr.srs1_3.bits.hook = 1; \
	} else if (_ch == 2) { \
		__context->status_mask.sr.srs1_2.bits.hook = 1; \
	} else if (_ch == 1) { \
		__context->status_mask.sr.srs1_1.bits.hook = 1; \
	} else { \
		__context->status_mask.sr.srs1_0.bits.hook = 1; \
	} \
} while (0)

#define vin_reset_hook_handler(_ctx, _ch) \
do { \
	struct vinetic_context *__context = (struct vinetic_context *)_ctx; \
	__context->vin_hook_handler[_ch].handler = NULL; \
	__context->vin_hook_handler[_ch].data = NULL; \
	if (_ch == 3) { \
		__context->status_mask.sr.srs1_3.bits.hook = 0; \
	} else if (_ch == 2) { \
		__context->status_mask.sr.srs1_2.bits.hook = 0; \
	} else if (_ch == 1) { \
		__context->status_mask.sr.srs1_1.bits.hook = 0; \
	} else { \
		__context->status_mask.sr.srs1_0.bits.hook = 0; \
	} \
} while (0)

extern void vin_state_dump(struct vinetic_context *ctx);

#endif //__LIBVINETIC_H__

/******************************************************************************/
/* end of libvinetic.h                                                        */
/******************************************************************************/
