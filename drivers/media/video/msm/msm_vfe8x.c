/*
 * Copyright (C) 2008 QUALCOMM Incorporated.
 * Author: Haibo Jeff Zhong <hzhong@qualcomm.com>
 *
 */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/arch/irqs.h>
#include "msm_vfe8x_proc.h"

#define ON  1
#define OFF 0

struct platform_device *vfe_pdev;

static int vfe_enable(struct camera_enable_cmd_t *enable)
{
	int rc = 0;

	return rc;
}

static int vfe_disable(struct camera_enable_cmd_t *enable)
{
	int rc = 0;

	vfe_stop();

	msm_camio_disable();
	return rc;
}

static void vfe_release(void)
{
	disable_irq(INT_VFE);
	free_irq(INT_VFE, 0);

	msm_camio_disable();
	vfe_cmd_release();
}

static irqreturn_t vfe_isr(int irq, void *data)
{
	vfe_parse_irq();
	return IRQ_HANDLED;
}

static void vfe_config_axi(enum vfeoutput_mode_t mode,
	struct axidata_t *ad, struct vfe_cmd_axi_output_config *ao)
{
	struct msm_pmem_region *regptr;
	int i, j;
	uint32_t *p1, *p2;

	if (mode == OUTPUT_1 || mode == OUTPUT_1_AND_2_SNAP) {
		regptr = ad->region;
		for (i = 0;
			i < ad->bufnum1; i++) {

			p1 = &(ao->output1.outputY.outFragments[i][0]);
			p2 = &(ao->output1.outputCbcr.outFragments[i][0]);

			for (j = 0;
				j < ao->output1.fragmentCount; j++) {

			*p1 = regptr->paddr + regptr->y_off;
			p1++;

			*p2 = regptr->paddr + regptr->cbcr_off;
			p2++;
		}
			regptr++;
		}
	} /* if OUTPUT1 or Both */

	if (mode == OUTPUT_2 || mode == OUTPUT_1_AND_2_SNAP) {

		regptr = &(ad->region[ad->bufnum1]);
		CDBG("bufnum2 = %d\n", ad->bufnum2);

		for (i = 0;
			i < ad->bufnum2; i++) {

			p1 = &(ao->output2.outputY.outFragments[i][0]);
			p2 = &(ao->output2.outputCbcr.outFragments[i][0]);

		CDBG("config_axi: O2, phy = 0x%lx, y_off = %d, cbcr_off = %d\n",
			regptr->paddr, regptr->y_off, regptr->cbcr_off);

			for (j = 0;
				j < ao->output2.fragmentCount; j++) {

			*p1 = regptr->paddr + regptr->y_off;
			CDBG("vfe_config_axi: p1 = 0x%x\n", *p1);
			p1++;

			*p2 = regptr->paddr + regptr->cbcr_off;
			CDBG("vfe_config_axi: p2 = 0x%x\n", *p2);
			p2++;
		}
			regptr++;
		}
	}
}

static int vfe_proc_general(struct msm_vfe_command_8k *cmd)
{
	int rc = 0;

	CDBG("vfe_proc_general: cmdID = %d\n", cmd->id);

	switch (cmd->id) {
	case VFE_CMD_ID_RESET:
		msm_camio_vfe_blk_reset();
		msm_camio_camif_pad_reg_reset_2();
		vfe_reset();
		break;

	case VFE_CMD_ID_START: {
		struct vfe_cmd_start start;
		if (copy_from_user(&start,
					cmd->value, cmd->length))
			rc = -EFAULT;

		msm_camio_camif_pad_reg_reset_2();
		vfe_start(&start);
	}
		break;

	case VFE_CMD_ID_CAMIF_CONFIG: {
		struct vfe_cmd_camif_config camif;
		if (copy_from_user(&camif,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_camif_config(&camif);
	}
		break;

	case VFE_CMD_ID_BLACK_LEVEL_CONFIG: {
		struct vfe_cmd_black_level_config bl;
		if (copy_from_user(&bl,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_black_level_config(&bl);
	}
		break;

	case VFE_CMD_ID_ROLL_OFF_CONFIG: {
		struct vfe_cmd_roll_off_config rolloff;
		if (copy_from_user(&rolloff,
				cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_roll_off_config(&rolloff);
	}
		break;

	case VFE_CMD_ID_DEMUX_CHANNEL_GAIN_CONFIG: {
		struct vfe_cmd_demux_channel_gain_config demuxc;
		if (copy_from_user(&demuxc,
					cmd->value, cmd->length))
			rc = -EFAULT;

		/* demux is always enabled.  */
		vfe_demux_channel_gain_config(&demuxc);
	}
		break;

	case VFE_CMD_ID_DEMOSAIC_CONFIG: {
		struct vfe_cmd_demosaic_config demosaic;
		if (copy_from_user(&demosaic,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_demosaic_config(&demosaic);
	}
		break;

	case VFE_CMD_ID_FOV_CROP_CONFIG:
	case VFE_CMD_ID_FOV_CROP_UPDATE: {
		struct vfe_cmd_fov_crop_config fov;
		if (copy_from_user(&fov,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_fov_crop_config(&fov);
	}
		break;

	case VFE_CMD_ID_MAIN_SCALER_CONFIG:
	case VFE_CMD_ID_MAIN_SCALER_UPDATE: {
		struct vfe_cmd_main_scaler_config mainds;
		if (copy_from_user(&mainds,
				cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_main_scaler_config(&mainds);
	}
		break;

	case VFE_CMD_ID_WHITE_BALANCE_CONFIG:
	case VFE_CMD_ID_WHITE_BALANCE_UPDATE: {
		struct vfe_cmd_white_balance_config wb;
		if (copy_from_user(&wb,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_white_balance_config(&wb);
	}
		break;

	case VFE_CMD_ID_COLOR_CORRECTION_CONFIG:
	case VFE_CMD_ID_COLOR_CORRECTION_UPDATE: {
		struct vfe_cmd_color_correction_config cc;
		if (copy_from_user(&cc,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_color_correction_config(&cc);
	}
		break;

	case VFE_CMD_ID_LA_CONFIG: {
		struct vfe_cmd_la_config la;
		if (copy_from_user(&la,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_la_config(&la);
	}
		break;

	case VFE_CMD_ID_RGB_GAMMA_CONFIG: {
		struct vfe_cmd_rgb_gamma_config rgb;
		if (copy_from_user(&rgb,
					cmd->value, cmd->length))
			rc = -EFAULT;

		rc = vfe_rgb_gamma_config(&rgb);
	}
		break;

	case VFE_CMD_ID_CHROMA_ENHAN_CONFIG:
	case VFE_CMD_ID_CHROMA_ENHAN_UPDATE: {
		struct vfe_cmd_chroma_enhan_config chrom;
		if (copy_from_user(&chrom,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_chroma_enhan_config(&chrom);
	}
		break;

	case VFE_CMD_ID_CHROMA_SUPPRESSION_CONFIG:
	case VFE_CMD_ID_CHROMA_SUPPRESSION_UPDATE: {
		struct vfe_cmd_chroma_suppression_config chromsup;
		if (copy_from_user(&chromsup,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_chroma_sup_config(&chromsup);
	}
		break;

	case VFE_CMD_ID_ASF_CONFIG: {
		struct vfe_cmd_asf_config asf;
		if (copy_from_user(&asf,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_asf_config(&asf);
	}
		break;

	case VFE_CMD_ID_SCALER2Y_CONFIG:
	case VFE_CMD_ID_SCALER2Y_UPDATE: {
		struct vfe_cmd_scaler2_config ds2y;
		if (copy_from_user(&ds2y,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_scaler2y_config(&ds2y);
	}
		break;

	case VFE_CMD_ID_SCALER2CbCr_CONFIG:
	case VFE_CMD_ID_SCALER2CbCr_UPDATE: {
		struct vfe_cmd_scaler2_config ds2cbcr;
		if (copy_from_user(&ds2cbcr,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_scaler2cbcr_config(&ds2cbcr);
	}
		break;

	case VFE_CMD_ID_CHROMA_SUBSAMPLE_CONFIG: {
		struct vfe_cmd_chroma_subsample_config sub;
		if (copy_from_user(&sub,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_chroma_subsample_config(&sub);
	}
		break;

	case VFE_CMD_ID_FRAME_SKIP_CONFIG: {
		struct vfe_cmd_frame_skip_config fskip;
		if (copy_from_user(&fskip,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_frame_skip_config(&fskip);
	}
		break;

	case VFE_CMD_ID_OUTPUT_CLAMP_CONFIG: {
		struct vfe_cmd_output_clamp_config clamp;
		if (copy_from_user(&clamp,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_output_clamp_config(&clamp);
	}
		break;

	/* module update commands */
	case VFE_CMD_ID_BLACK_LEVEL_UPDATE: {
		struct vfe_cmd_black_level_config blk;
		if (copy_from_user(&blk,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_black_level_update(&blk);
	}
		break;

	case VFE_CMD_ID_DEMUX_CHANNEL_GAIN_UPDATE: {
		struct vfe_cmd_demux_channel_gain_config dmu;
		if (copy_from_user(&dmu,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_demux_channel_gain_update(&dmu);
	}
		break;

	case VFE_CMD_ID_DEMOSAIC_BPC_UPDATE: {
		struct vfe_cmd_demosaic_bpc_update demo_bpc;
		if (copy_from_user(&demo_bpc,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_demosaic_bpc_update(&demo_bpc);
	}
		break;

	case VFE_CMD_ID_DEMOSAIC_ABF_UPDATE: {
		struct vfe_cmd_demosaic_abf_update demo_abf;
		if (copy_from_user(&demo_abf,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_demosaic_abf_update(&demo_abf);
	}
		break;

	case VFE_CMD_ID_LA_UPDATE: {
		struct vfe_cmd_la_config la;
		if (copy_from_user(&la,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_la_update(&la);
	}
		break;

	case VFE_CMD_ID_RGB_GAMMA_UPDATE: {
		struct vfe_cmd_rgb_gamma_config rgb;
		if (copy_from_user(&rgb,
					cmd->value, cmd->length))
			rc = -EFAULT;

		rc = vfe_rgb_gamma_update(&rgb);
	}
		break;

	case VFE_CMD_ID_ASF_UPDATE: {
		struct vfe_cmd_asf_update asf;
		if (copy_from_user(&asf,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_asf_update(&asf);
	}
		break;

	case VFE_CMD_ID_FRAME_SKIP_UPDATE: {
		struct vfe_cmd_frame_skip_update fskip;
		if (copy_from_user(&fskip,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_frame_skip_update(&fskip);
	}
		break;

	case VFE_CMD_ID_CAMIF_FRAME_UPDATE: {
		struct vfe_cmds_camif_frame fup;
		if (copy_from_user(&fup,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_camif_frame_update(&fup);
	}
		break;

	/* stats update commands */
	case VFE_CMD_ID_STATS_AUTOFOCUS_UPDATE: {
		struct vfe_cmd_stats_af_update afup;
		if (copy_from_user(&afup,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_stats_update_af(&afup);
	}
		break;

	case VFE_CMD_ID_STATS_WB_EXP_UPDATE: {
		struct vfe_cmd_stats_wb_exp_update wbexp;
		if (copy_from_user(&wbexp,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_stats_update_wb_exp(&wbexp);
	}
		break;

	/* control of start, stop, update, etc... */
	case VFE_CMD_ID_STOP:
		vfe_stop();
		break;

	case VFE_CMD_ID_GET_HW_VERSION:
		break;

	/* stats */
	case VFE_CMD_ID_STATS_SETTING: {
		struct vfe_cmd_stats_setting stats;
		if (copy_from_user(&stats,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_stats_setting(&stats);
	}
		break;

	case VFE_CMD_ID_STATS_AUTOFOCUS_START: {
		struct vfe_cmd_stats_af_start af;
		if (copy_from_user(&af,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_stats_start_af(&af);
	}
		break;

	case VFE_CMD_ID_STATS_AUTOFOCUS_STOP:
		vfe_stats_af_stop();
		break;

	case VFE_CMD_ID_STATS_WB_EXP_START: {
		struct vfe_cmd_stats_wb_exp_start awexp;
		if (copy_from_user(&awexp,
					cmd->value, cmd->length))
			rc = -EFAULT;

		vfe_stats_start_wb_exp(&awexp);
	}
		break;

	case VFE_CMD_ID_STATS_WB_EXP_STOP:
		vfe_stats_wb_exp_stop();
		break;

	case VFE_CMD_ID_ASYNC_TIMER_SETTING:
		break;

	case VFE_CMD_ID_UPDATE:
		vfe_update();
		break;

	/* test gen */
	case VFE_CMD_ID_TEST_GEN_START:
		break;

/*
  ackownledge from upper layer
	these are not in general command.

	case VFE_CMD_ID_OUTPUT1_ACK:
		break;
	case VFE_CMD_ID_OUTPUT2_ACK:
		break;
	case VFE_CMD_ID_EPOCH1_ACK:
		break;
	case VFE_CMD_ID_EPOCH2_ACK:
		break;
	case VFE_CMD_ID_STATS_AUTOFOCUS_ACK:
		break;
	case VFE_CMD_ID_STATS_WB_EXP_ACK:
		break;
*/

	default:
		break;
	} /* switch */

	return rc;
}

static int vfe_config(struct msm_vfe_cfg_cmd_t *cmd, void *data)
{
	struct msm_pmem_region *regptr;
	struct msm_vfe_command_8k vfecmd;

	uint32_t i;

	void *cmd_data = NULL;
	long rc = 0;

	struct vfe_cmd_axi_output_config *axio = NULL;
	struct vfe_cmd_stats_setting *scfg_t = NULL;

	if (cmd->cmd_type != CMD_FRAME_BUF_RELEASE &&
	    cmd->cmd_type != CMD_STATS_BUF_RELEASE) {

		if (copy_from_user(&vfecmd,
				(void *)(cmd->value),
				sizeof(struct msm_vfe_command_8k)))
			return -EFAULT;
	}

	CDBG("vfe_config: cmdType = %d\n", cmd->cmd_type);

	switch (cmd->cmd_type) {
	case CMD_GENERAL:
		rc = vfe_proc_general(&vfecmd);
		break;

	case CMD_STATS_ENABLE:
	case CMD_STATS_AXI_CFG: {
		struct axidata_t *axid;

		axid = (struct axidata_t *)data;
		if (!axid)
			return -EFAULT;

		scfg_t =
			kmalloc(sizeof(struct vfe_cmd_stats_setting),
				GFP_ATOMIC);
		if (!scfg_t)
			return -ENOMEM;

		if (copy_from_user(scfg_t,
					(void *)(vfecmd.value),
					vfecmd.length)) {

			kfree(scfg_t);
			return -EFAULT;
		}

		regptr = axid->region;
		if (axid->bufnum1 > 0) {
			for (i = 0; i < axid->bufnum1; i++) {
				scfg_t->awbBuffer[i] =
					(uint32_t)(regptr->paddr);
				regptr++;
			}
		}

		if (axid->bufnum2 > 0) {
			for (i = 0; i < axid->bufnum1; i++) {
				scfg_t->afBuffer[i] =
					(uint32_t)(regptr->paddr);
				regptr++;
			}
		}

		vfe_stats_config(scfg_t);
	}
		break;

	case CMD_STATS_AF_AXI_CFG: {
	}
		break;

	case CMD_FRAME_BUF_RELEASE: {
		/* preview buffer release */
		struct msm_frame_t *b;
		unsigned long p;
		struct vfe_cmd_output_ack fack;

		if (!data)
			return -EFAULT;

		b = (struct msm_frame_t *)(cmd->value);
		p = *(unsigned long *)data;

		b->path = MSM_FRAME_ENC;

		fack.ybufaddr[0] =
			(uint32_t)(p + b->y_off);

		fack.chromabufaddr[0] =
			(uint32_t)(p + b->cbcr_off);

		if (b->path == MSM_FRAME_PREV)
			vfe_output1_ack(&fack);

		if (b->path == MSM_FRAME_ENC)
			vfe_output2_ack(&fack);
	}
		break;

	case CMD_SNAP_BUF_RELEASE: {
	}
		break;

	case CMD_STATS_BUF_RELEASE: {
		struct vfe_cmd_stats_wb_exp_ack sack;

		if (!data)
			return -EFAULT;

		sack.nextWbExpOutputBufferAddr = *(uint32_t *)data;
		vfe_stats_wb_exp_ack(&sack);
	}
		break;

	case CMD_AXI_CFG_OUT1: {
		struct axidata_t *axid;

		axid = (struct axidata_t *)data;
		if (!axid)
			return -EFAULT;

		axio =
			kmalloc(sizeof(struct vfe_cmd_axi_output_config),
				GFP_ATOMIC);
		if (!axio)
			return -ENOMEM;

		if (copy_from_user(axio, (void *)(vfecmd.value),
			sizeof(struct vfe_cmd_axi_output_config))) {
			kfree(axio);
			return -EFAULT;
		}

		vfe_config_axi(OUTPUT_1, axid, axio);
		vfe_axi_output_config(axio);
	}
		break;

	case CMD_AXI_CFG_OUT2:
	case CMD_RAW_PICT_AXI_CFG: {
		struct axidata_t *axid;

		axid = (struct axidata_t *)data;
		if (!axid)
			return -EFAULT;

		axio =
			kmalloc(sizeof(struct vfe_cmd_axi_output_config),
				GFP_ATOMIC);
		if (!axio)
			return -ENOMEM;

		if (copy_from_user(axio, (void *)(vfecmd.value),
				sizeof(struct vfe_cmd_axi_output_config))) {
			kfree(axio);
			return -EFAULT;
		}

		vfe_config_axi(OUTPUT_2, axid, axio);

		axio->outputDataSize = 0;
		vfe_axi_output_config(axio);
	}
		break;

	case CMD_AXI_CFG_SNAP_O1_AND_O2: {
		struct axidata_t *axid;
		axid = (struct axidata_t *)data;
		if (!axid)
			return -EFAULT;

		axio =
			kmalloc(sizeof(struct vfe_cmd_axi_output_config),
				GFP_ATOMIC);
		if (!axio)
			return -ENOMEM;

		if (copy_from_user(axio, (void *)(vfecmd.value),
			sizeof(struct vfe_cmd_axi_output_config))) {
			kfree(axio);
			return -EFAULT;
		}

		vfe_config_axi(OUTPUT_1_AND_2_SNAP,
			axid, axio);
		vfe_axi_output_config(axio);
		cmd_data = axio;
	}
		break;

	default:
		break;
	} /* switch */

	kfree(scfg_t);

	kfree(axio);

/*
	if (cmd->length > 256 &&
			cmd_data &&
			(cmd->cmd_type == CMD_GENERAL ||
			 cmd->cmd_type == CMD_STATS_DISABLE)) {
		kfree(cmd_data);
	}
*/
	return rc;
}

static int vfe_init(struct msm_vfe_resp *presp)
{
	int rc = 0;

	rc = vfe_cmd_init(presp, vfe_pdev);
	if (rc < 0)
		return rc;

	/* Bring up all the required GPIOs and Clocks */
	//msm_camio_enable(dev);
	msm_camio_camif_pad_reg_reset();

	/* Move this to msm_vfe8x_proc.c */
	rc = request_irq(INT_VFE, vfe_isr, IRQF_TRIGGER_RISING,
		"vfe", 0);
	if (rc < 0)
		return rc;

	enable_irq(INT_VFE);

	return 0;
}

void msm_camvfe_init(struct msm_camvfe_fn_t *fptr,
	struct platform_device *dev)
{
	fptr->vfe_init    = vfe_init;
	fptr->vfe_enable  = vfe_enable;
	fptr->vfe_config  = vfe_config;
	fptr->vfe_disable = vfe_disable;
	fptr->vfe_release = vfe_release;

	vfe_pdev = dev;
}
