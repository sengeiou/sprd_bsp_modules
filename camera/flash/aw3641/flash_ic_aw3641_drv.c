/*
 * Copyright (C) 2015-2016 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "%s:%d " fmt, __func__, __LINE__

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include "sprd_img.h"
#include "flash_drv.h"


#define I2C_SLAVEADDR	0x63
#define FLASH_IC_DRIVER_NAME	"sprd_aw3641"
#define FLASH_IC_GPIO_MAX 2

struct flash_ic_cfg {
	unsigned int lvfm_enable;
	unsigned int torch_level;
	unsigned int preflash_level;
	unsigned int highlight_level;
	unsigned int cfg_factor; /*factor for isp level(32) to real level(128)*/
};

struct flash_driver_data {
	struct i2c_client *i2c_info;
	struct mutex i2c_lock;
	int gpio_tab[FLASH_IC_GPIO_MAX];
	void *priv;
	struct flash_ic_cfg flash_cfg;
	u32 torch_led_index;
};

static const char *const flash_ic_gpio_names[FLASH_IC_GPIO_MAX] = {
	"flash-torch-en-gpios", /* for torch/flash mode */
	"flash-en-gpios", /* for enable ic pin */
};


static int sprd_flash_ic_init(void *drvd)
{
	int ret = 0;
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

	if (!drv_data)
		return -EFAULT;

	return ret;
}

static int sprd_flash_ic_open_torch(void *drvd, uint8_t idx)
{
	int ret = 0;
	int gpio_id = 0;
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

	if (!drv_data)
		return -EFAULT;

	idx = drv_data->torch_led_index;
	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_ON);
			if (ret)
				goto exit;
		}
	}

	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_ON);
			if (ret)
				goto exit;
		}
	}

exit:
	return ret;
}

static int sprd_flash_ic_close_torch(void *drvd, uint8_t idx)
{
	int ret = 0;
	int gpio_id = 0;
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

	if (!drv_data)
		return -EFAULT;

	idx = drv_data->torch_led_index;
	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}


	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}

exit:
	return 0;
}

static int sprd_flash_ic_open_preflash(void *drvd, uint8_t idx)
{
	int ret = 0;
	int gpio_id = 0;
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

	if (!drv_data)
		return -EFAULT;

	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_ON);
			if (ret)
				goto exit;
		}
	}

	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_ON);
			if (ret)
				goto exit;
		}
	}
exit:
	return ret;
}

static int sprd_flash_ic_close_preflash(void *drvd, uint8_t idx)
{
	int ret = 0;
	int gpio_id = 0;
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

	if (!drv_data)
		return -EFAULT;

	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}

	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}
exit:
	return ret;
}

static int sprd_flash_ic_open_highlight(void *drvd, uint8_t idx)
{
	int ret = 0;
	int gpio_id = 0;
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

	if (!drv_data)
		return -EFAULT;

	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[1];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_ON);
			if (ret)
				goto exit;
		}
	}

	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[1];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_ON);
			if (ret)
				goto exit;
		}
	}
exit:
	return ret;
}

static int sprd_flash_ic_close_highlight(void *drvd, uint8_t idx)
{
	int ret = 0;
	int gpio_id = 0;
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

	if (!drv_data)
		return -EFAULT;

	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[1];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}

	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[1];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}
exit:
	return ret;
}

static int sprd_flash_ic_cfg_value_torch(void *drvd, uint8_t idx,
					  struct sprd_flash_element *element)
{
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
	int gpio_id = 0;
	int ret = 0;

	if (!drv_data)
		return -EFAULT;

	idx = drv_data->torch_led_index;
	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}
	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}
exit:
	return ret;
}

static int sprd_flash_ic_cfg_value_preflash(void *drvd, uint8_t idx,
					  struct sprd_flash_element *element)
{
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
	int ret = 0;
	int gpio_id = 0;

	if (!drv_data)
		return -EFAULT;

	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}

	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
			if (ret)
				goto exit;
		}
	}
exit:
	return ret;
}

static int sprd_flash_ic_cfg_value_highlight(void *drvd, uint8_t idx,
					   struct sprd_flash_element *element)
{
	struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
	int ret = 0;
	int gpio_id = 0;

	if (!drv_data)
		return -EFAULT;

	if (SPRD_FLASH_LED0 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_ON);
			if (ret)
				goto exit;
		}
	}

	if (SPRD_FLASH_LED1 & idx) {
		gpio_id = drv_data->gpio_tab[0];
		if (gpio_is_valid(gpio_id)) {
			ret = gpio_direction_output(gpio_id, SPRD_FLASH_ON);
			if (ret)
				goto exit;
		}
	}
exit:
	return ret;
}

static const struct of_device_id sprd_flash_ic_of_match_table[] = {
	{.compatible = "sprd,flash-aw3641"},
};

static const struct i2c_device_id sprd_flash_ic_ids[] = {
	{}
};
static const struct sprd_flash_driver_ops flash_ic_ops = {
	.open_torch = sprd_flash_ic_open_torch,
	.close_torch = sprd_flash_ic_close_torch,
	.open_preflash = sprd_flash_ic_open_preflash,
	.close_preflash = sprd_flash_ic_close_preflash,
	.open_highlight = sprd_flash_ic_open_highlight,
	.close_highlight = sprd_flash_ic_close_highlight,
	.cfg_value_torch = sprd_flash_ic_cfg_value_torch,
	.cfg_value_preflash = sprd_flash_ic_cfg_value_preflash,
	.cfg_value_highlight = sprd_flash_ic_cfg_value_highlight,

};

static int sprd_flash_ic_driver_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret = 0;
	struct device *dev = &client->dev;
	struct flash_driver_data *pdata = NULL;
	unsigned int j;

	int gpio[FLASH_IC_GPIO_MAX];
	unsigned int torch = 0, preflash = 0, highlight = 0;

	if (!dev->of_node) {
		pr_err("no device node %s", __func__);
		return -ENODEV;
	}
	pr_info("flash-ic-driver probe\n");
	ret = of_property_read_u32(dev->of_node, "sprd,flash-ic", &j);
	if (ret || j != 3641)
		return -ENODEV;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;
	client->dev.platform_data = (void *)pdata;
	pdata->i2c_info = client;
	mutex_init(&pdata->i2c_lock);
	ret = of_property_read_u32(dev->of_node, "sprd,torch", &torch);
	if (ret)
		goto exit;

	ret = of_property_read_u32(dev->of_node,
				   "sprd,preflash", &preflash);
	if (ret)
		goto exit;

	ret = of_property_read_u32(dev->of_node,
				   "sprd,highlight", &highlight);

	ret = of_property_read_u32(dev->of_node,
			"sprd,torch-level", &pdata->flash_cfg.torch_level);
	if (ret)
		pr_info("torch-level no cfg\n");

	ret = of_property_read_u32(dev->of_node,
		"sprd,preflash-level", &pdata->flash_cfg.preflash_level);
	if (ret)
		pr_info("preflash-level no cfg\n");

	ret = of_property_read_u32(dev->of_node,
		"sprd,highlight-level", &pdata->flash_cfg.highlight_level);
	if (ret)
		pr_info("highlight-level no cfg\n");
	ret = of_property_read_u32(dev->of_node,
			 "sprd,lvfm-enable", &pdata->flash_cfg.lvfm_enable);
	if (ret)
		pr_info("lvfm-enable no cfg\n");
	ret = of_property_read_u32(dev->of_node,
				"torch-led-idx", &pdata->torch_led_index);
	if (ret)
		pdata->torch_led_index = SPRD_FLASH_LED0;

	for (j = 0; j < FLASH_IC_GPIO_MAX; j++) {
		gpio[j] = of_get_named_gpio(dev->of_node,
					       flash_ic_gpio_names[j], 0);
		if (gpio_is_valid(gpio[j])) {
			ret = devm_gpio_request(dev,
						gpio[j],
						flash_ic_gpio_names[j]);
			if (ret)
				pr_info("flash gpio err\n");
		}
	}

	memcpy((void *)pdata->gpio_tab, (void *)gpio, sizeof(gpio));

	ret = sprd_flash_ic_init(pdata);

	ret = sprd_flash_register(&flash_ic_ops, pdata, SPRD_FLASH_REAR);

exit:
	return ret;
}
static int sprd_flash_ic_driver_remove(struct i2c_client *client)
{
	struct flash_driver_data *pdata = NULL;

	pdata = (struct flash_driver_data *)client->dev.platform_data;
	if (pdata)
		devm_kfree(&client->dev, pdata);

	pdata = NULL;
	client->dev.platform_data = NULL;

	return 0;
}

static struct i2c_driver sprd_flash_ic_driver = {
	.driver = {
		.of_match_table = of_match_ptr(sprd_flash_ic_of_match_table),
		.name = FLASH_IC_DRIVER_NAME,
		},
	.probe = sprd_flash_ic_driver_probe,
	.remove = sprd_flash_ic_driver_remove,
	.id_table = sprd_flash_ic_ids,
};

static int sprd_flash_ic_register_driver(void)
{
	int ret = 0;

	ret = i2c_add_driver(&sprd_flash_ic_driver);
	pr_info("register sprd_flash_ic_driver:%d\n", ret);

	return ret;
}

static void sprd_flash_ic_unregister_driver(void)
{
	i2c_del_driver(&sprd_flash_ic_driver);
}

static int sprd_flash_ic_driver_init(void)
{
	int ret = 0;

	ret = sprd_flash_ic_register_driver();
	return ret;
}

static void sprd_flash_ic_driver_deinit(void)
{
	sprd_flash_ic_unregister_driver();
}

module_init(sprd_flash_ic_driver_init);
module_exit(sprd_flash_ic_driver_deinit);
MODULE_DESCRIPTION("Sprd aw3641 Flash Driver");
MODULE_LICENSE("GPL");
