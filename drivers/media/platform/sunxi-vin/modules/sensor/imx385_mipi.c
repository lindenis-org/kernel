/*
 * A V4L2 driver for imx385 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h> 
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for IMX385 sensors");
MODULE_LICENSE("GPL");

#define MCLK              37125000
#define V4L2_IDENT_SENSOR  0x0385

#define VMAX 	1125	
#define HMAX 	4400		
#define HMAX_50 2200

#define DOL_RHS1	51	//39
#define DOL_RATIO	16
#define Y_SIZE		2204
/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The IMX385 i2c address
 */
#define I2C_ADDR 0x34

#define SENSOR_NUM		0x2
#define SENSOR_NAME		"imx385_mipi"
#define SENSOR_NAME_2	"imx385_mipi_2"

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_4lane_1080P30_regs[] = {
	{0x3000, 0x01},
		
	{0x3009, 0x02},
	{0x310B, 0x07},
	{0x3110, 0x12},
	{0x3012, 0x2C},
	{0x3013, 0x01},
	{0x301B, 0x30},
	{0x301C, 0x11},

	{0x3338, 0xD4},
	{0x3339, 0x40},
	{0x333A, 0x10},
	{0x333B, 0x00},
	{0x333C, 0xD4},
	{0x333D, 0x40},
	{0x333E, 0x10},
	{0x333F, 0x00},
	{0x3344, 0x10},
	
	{0x3049, 0x0A},
	{0x3054, 0x66},
	{0x305D, 0x00},
	{0x305F, 0x00},
	{0x336B, 0x2F},   //THSEXIT
	{0x336C, 0x1F}, //TCLKPRE
	{0x3380, 0x20},
	{0x3381, 0x25},
	{0x3382, 0x5F},
	{0x3383, 0x17},
	{0x3384, 0x2F},
	{0x3385, 0x37},   //HS-trail
	{0x3386, 0x17},
	{0x3387, 0x0F},
	{0x3388, 0x4F},
	
	{0x338D, 0xB4},
	{0x338E, 0x01},
	{0x31ED, 0x38},
	
	{0x3014, 0x58}, //GAIN
	{0x3015, 0x00},
	
	{0x3000, 0x00},
	{0x3002, 0x00}, //master mode operation start

};

static struct regval_list sensor_4lane_1080P30DOL_regs[] = {
	{0x3000, 0x01},
	
	{0x3007, 0x10},
	{0x300C, 0x11},
	{0x3012, 0x2C},
	{0x3013, 0x01},
	{0x301B, HMAX_50&0xFF},
	{0x301C, HMAX_50>>8},
	{0x3020, 0x03},
	{0x3023, 0xE9},
	{0x3024, 0x03},
	{0x302C, DOL_RHS1},
	{0x3043, 0x05},
	{0x3049, 0x0A},
	{0x3054, 0x66},
	{0x305D, 0x00},
	{0x305F, 0x00},
	
	{0x3109, 0x01},
	{0x310B, 0x07},
	{0x3110, 0x12},
	{0x31ED, 0x38},
	
	{0x3338, 0xD4},
	{0x3339, 0x40},
	{0x333A, 0x10},
	{0x333B, 0x00},
	{0x333C, 0xD4},
	{0x333D, 0x40},
	{0x333E, 0x10},
	{0x333F, 0x00},
	{0x3346, 0x03},
	{0x3353, 0x0E},
	{0x3354, 0x00},
	{0x3357, 0xB2},
	{0x3358, 0x08},
	{0x337D, 0x0C},
	{0x337E, 0x0C},
	{0x3380, 0x20},
	{0x3381, 0x25},
	{0x338D, 0xB4},
	{0x338E, 0x01},
	
	{0x3014, 0x58}, //GAIN
	{0x3015, 0x00},
	
	{0x3000, 0x00},
	{0x3002, 0x00}, //master mode operation start

};



/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int imx385_sensor_vts;
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, expmid, exphigh;
	int shttime,  exp_val_m, exp_val_s;
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	shttime = wsize->vts - (exp_val >> 4) - 1;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) 
	{
		// LEF
		shttime = (imx385_sensor_vts<<1) - (exp_val>>4) - 1;
		if (shttime < DOL_RHS1 + 2)
		{
			shttime = DOL_RHS1 + 2;
			exp_val = ((imx385_sensor_vts<<1) - shttime - 1) << 4;
		}
		sensor_dbg("long exp_val: %d, shttime: %d\n", exp_val, shttime);
		
		exphigh = (unsigned char) ((0x0030000&shttime)>>16);
		expmid	= (unsigned char) ((0x000ff00&shttime)>>8 );
		explow	= (unsigned char) ((0x00000ff&shttime)	  );
			
		sensor_write(sd, 0x3023, explow);
		sensor_write(sd, 0x3024, expmid);
		sensor_write(sd, 0x3025, exphigh);

		// SEF
		exp_val_m = exp_val/DOL_RATIO;
		if (exp_val_m < 2 * 16)
			exp_val_m = 2 * 16;
		shttime = DOL_RHS1 - (exp_val_m>>4) - 1;
		if (shttime < 1)
			shttime = 1;

		sensor_dbg("short exp_val: %d, shttime: %d\n", exp_val_m, shttime);

		exphigh = (unsigned char) ((0x0030000&shttime)>>16);
		expmid	= (unsigned char) ((0x000ff00&shttime)>>8 );
		explow	= (unsigned char) ((0x00000ff&shttime)	  );

		sensor_write(sd, 0x3020, explow);
		sensor_write(sd, 0x3021, expmid);
		sensor_write(sd, 0x3022, exphigh);
	} 
	else
	{
		exphigh = (unsigned char)((0x0030000 & shttime) >> 16);
		expmid = (unsigned char)((0x000ff00 & shttime) >> 8);
		explow = (unsigned char)((0x00000ff & shttime));
		sensor_write(sd, 0x3020, explow);
		sensor_write(sd, 0x3021, expmid);
		sensor_write(sd, 0x3022, exphigh);
	}
	sensor_dbg("sensor_set_exp = %d %d %d %d Done!\n", imx385_sensor_vts, exp_val, shttime, exp_val_m);


	info->exp = exp_val;
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

unsigned char gain2db[497] = {
	0,   2,   3,	 5,   6,   8,	9,  11,  12,  13,  14,	15,  16,  17,
	18,  19,  20,	21,  22,  23,  23,  24,  25,  26,  27,	27,  28,  29,
	29,  30,  31,	31,  32,  32,  33,  34,  34,  35,  35,	36,  36,  37,
	37,  38,  38,	39,  39,  40,  40,  41,  41,  41,  42,	42,  43,  43,
	44,  44,  44,	45,  45,  45,  46,  46,  47,  47,  47,	48,  48,  48,
	49,  49,  49,	50,  50,  50,  51,  51,  51,  52,  52,	52,  52,  53,
	53,  53,  54,	54,  54,  54,  55,  55,  55,  56,  56,	56,  56,  57,
	57,  57,  57,	58,  58,  58,  58,  59,  59,  59,  59,	60,  60,  60,
	60,  60,  61,	61,  61,  61,  62,  62,  62,  62,  62,	63,  63,  63,
	63,  63,  64,	64,  64,  64,  64,  65,  65,  65,  65,	65,  66,  66,
	66,  66,  66,	66,  67,  67,  67,  67,  67,  68,  68,	68,  68,  68,
	68,  69,  69,	69,  69,  69,  69,  70,  70,  70,  70,	70,  70,  71,
	71,  71,  71,	71,  71,  71,  72,  72,  72,  72,  72,	72,  73,  73,
	73,  73,  73,	73,  73,  74,  74,  74,  74,  74,  74,	74,  75,  75,
	75,  75,  75,	75,  75,  75,  76,  76,  76,  76,  76,	76,  76,  77,
	77,  77,  77,	77,  77,  77,  77,  78,  78,  78,  78,	78,  78,  78,
	78,  79,  79,	79,  79,  79,  79,  79,  79,  79,  80,	80,  80,  80,
	80,  80,  80,	80,  80,  81,  81,  81,  81,  81,  81,	81,  81,  81,
	82,  82,  82,	82,  82,  82,  82,  82,  82,  83,  83,	83,  83,  83,
	83,  83,  83,	83,  83,  84,  84,  84,  84,  84,  84,	84,  84,  84,
	84,  85,  85,	85,  85,  85,  85,  85,  85,  85,  85,	86,  86,  86,
	86,  86,  86,	86,  86,  86,  86,  86,  87,  87,  87,	87,  87,  87,
	87,  87,  87,	87,  87,  88,  88,  88,  88,  88,  88,	88,  88,  88,
	88,  88,  88,	89,  89,  89,  89,  89,  89,  89,  89,	89,  89,  89,
	89,  90,  90,	90,  90,  90,  90,  90,  90,  90,  90,	90,  90,  91,
	91,  91,  91,	91,  91,  91,  91,  91,  91,  91,  91,	91,  92,  92,
	92,  92,  92,	92,  92,  92,  92,  92,  92,  92,  92,	93,  93,  93,
	93,  93,  93,	93,  93,  93,  93,  93,  93,  93,  93,	94,  94,  94,
	94,  94,  94,	94,  94,  94,  94,  94,  94,  94,  94,	95,  95,  95,
	95,  95,  95,	95,  95,  95,  95,  95,  95,  95,  95,	95,  96,  96,
	96,  96,  96,	96,  96,  96,  96,  96,  96,  96,  96,	96,  96,  97,
	97,  97,  97,	97,  97,  97,  97,  97,  97,  97,  97,	97,  97,  97,
	97,  98,  98,	98,  98,  98,  98,  98,  98,  98,  98,	98,  98,  98,
	98,  98,  98,	99,  99,  99,  99,  99,  99,  99,  99,	99,  99,  99,
	99,  99,  99,	99,  99,  99, 100, 100, 100, 100, 100, 100, 100, 100,
	100, 100, 100, 100, 100, 100, 100,
};
static char gain_mode_buf = 0x02;
static unsigned int count;
static unsigned int gain_last = 0;;

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	int ret;
	data_type rdval;
	char gain_mode = 0x02;
	unsigned short gain_reg = 0;

	ret = sensor_read(sd, 0x3009, &rdval);
	if (ret != 0)
		return ret;
	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 16 * 16)
	{
		gain_mode = rdval | 0x10;
		if (gain_val < 64 * 16)
		{
			gain_reg = gain2db[(gain_val>>1) - 16];
		}
		else if (gain_val < 2048 * 16)
		{
			gain_reg = gain2db[(gain_val>>6) - 16] + 100;
		}
		else if (gain_val < 65536 * 16)
		{
			gain_reg = gain2db[(gain_val>>11) - 16] + 200;
		}
		else
		{
			gain_reg = gain2db[(gain_val>>16) - 16] + 300;		
		}
	} 
	else {
		gain_mode = rdval & 0xef;
		gain_reg = gain2db[gain_val - 16];
	}
	gain_reg *= 3;
	sensor_write(sd, 0x3014, gain_last & 0xff);
	sensor_write(sd, 0x3015, gain_last >> 8);	
	gain_last = gain_reg;
//	if (0 != (count++))
		sensor_write(sd, 0x3009, gain_mode);
//	gain_mode_buf = gain_mode;
	sensor_dbg("sensor_set_gain = %d, Done!\n", gain_val);
	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	int exp_val, gain_val;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	sensor_dbg("sensor_set_gain exp = %d, %d Done!\n", gain_val, exp_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x3000, &rdval);
	if (ret != 0)
		return ret;

	if (on_off == STBY_ON)
		ret = sensor_write(sd, 0x3000, rdval & 0xfe);
	else
		ret = sensor_write(sd, 0x3000, rdval | 0x01);

	return ret;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;

	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(10000, 12200);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(3000, 3200);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		vin_gpio_set_status(sd, POWER_EN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(100, 120);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval = 0;
	sensor_read(sd, 0x3008, &rdval);
	sensor_print("%s read value is 0x%x\n", __func__, rdval);
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = HD1080_WIDTH;
	info->height = HD1080_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
				sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		ret = 0;
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.desc = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {

	/*
	{
	 .width = 1936,
	 .height = 1096,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = HMAX,
	 .vts = VMAX,
	 .pclk = 148 * 1000 * 1000,
	 .mipi_bps = 446 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 1 << 4,
	 .intg_max = (VMAX - 3) << 4,
	 .gain_min = 1 << 4,
     .gain_max   = 65536<<4,
     .vipp_hoff = 8,
     .vipp_voff = 8,     
	 .regs = sensor_4lane_1080P30_regs,
	 .regs_size = ARRAY_SIZE(sensor_4lane_1080P30_regs),
	 .set_size = NULL,
	 },
	 */
 
	 {
	  .width = 1920,
	  .height = 1080,
	  .hoffset = 0,
	  .voffset = 1,
	  .hts = HMAX_50,
	  .vts = VMAX,
	  .pclk = 148 * 1000 * 1000,
	  .mipi_bps = 446 * 1000 * 1000,
	  .fps_fixed = 30,
	  .if_mode = MIPI_DOL_WDR_MODE,
	  .wdr_mode = ISP_DOL_WDR_MODE,
	  .bin_factor = 2,
	  .intg_min = 1 << 4,
	  .intg_max = (VMAX - 4) << 4,
	  .gain_min = 1 << 4,
	  .gain_max = 1440 << 4,
	  .regs = sensor_4lane_1080P30DOL_regs,
	  .regs_size = ARRAY_SIZE(sensor_4lane_1080P30DOL_regs),
	  .set_size = NULL,
	  },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_CSI2;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE)
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	else
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */

	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1*16, 65536*16, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 1, 65536 * 16, 1, 1);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0)
		return ret;

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum)
		return -ERANGE;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	imx385_sensor_vts = wsize->vts;

	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		     wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		     info->current_wins->width,
		     info->current_wins->height, info->fmt->mbus_code);

	if (!enable)
		return 0;

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}
};

static int sensor_dev_id;

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[i]);
	} else {
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[sensor_dev_id++]);
	}

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;

//	info->combo_mode = MIPI_NORMAL_MODE;	
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	int i;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	}, {
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME_2,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	},
};
static __init int init_sensor(void)
{
	int i, ret = 0;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		ret = cci_dev_init_helper(&sensor_driver[i]);

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

module_init(init_sensor);
module_exit(exit_sensor);
