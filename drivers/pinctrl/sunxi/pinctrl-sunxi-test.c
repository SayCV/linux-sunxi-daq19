/*
 * Allwinner SoCs pinctrl driver.
 *
 * Copyright (C) 2013 Shaorui Huang
 *
 * Shaorui Huang<huangshr@allwinnertech.com>
 * 2013-06-10  add sunxi pinctrl testing case.
 *
 * WimHuang<huangwei@allwinnertech.com>
 * 2015-07-20  transplant it from linux-3.4 to linux-3.10.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/io.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/sys_config.h>

#include "../core.h"

#define SUNXI_DEV_NAME_MAX_LEN		20
#define SUNXI_FUNC_NAME_MAX_LEN		80
#define NUM_ELEMS(a)		(sizeof(a)/sizeof((a)[0]))

struct sunxi_pctrltest_data {
	char dev_name[SUNXI_DEV_NAME_MAX_LEN];
	char exec[SUNXI_FUNC_NAME_MAX_LEN];
	int result;
	int gpio_index;
	int funcs;
	int pull;
	int data;
	int dlevel;
};

struct sunxi_pctrltest_case {
	const char *name;
	int (* func)(void);
};

/* sunxi gpio config info */
struct sunxi_gpio_config {
	const char *name;	/* gpio name */
	u32 mulsel;		/* multi sel val: 0 - input, 1 - output... */
	u32 pull;		/* pull val: 0 - pull up/down disable, 1 - pull up... */
	u32 drive;		/* driver level val: 0 - level 0, 1 - level 1... */
	u32 data;		/* data val: 0 - low, 1 - high, only vaild when mul_sel is input/output */
};

static struct sunxi_pctrltest_data *sunxi_ptest_data;

static int dt_node_to_gpio(struct device_node *np_cfg,
			   struct sunxi_gpio_config **gpio_list,
			   unsigned *gpio_count)
{
	struct property *prop;
	const char *name;
	int val;
	int i = 0;

	*gpio_count = of_property_count_strings(np_cfg, "allwinner,pins");
	if (*gpio_count < 0) {
		pr_warn("missing allwinner,pins property in node %s\n",
			 np_cfg->name);
		return -EINVAL;
	}

	*gpio_list = kmalloc(*gpio_count * sizeof(struct sunxi_gpio_config), GFP_KERNEL);
	if (!*gpio_list) {
		pr_warn("No enougt memory for gpio_list\n");
		return -ENOMEM;
	}

	of_property_for_each_string(np_cfg, "allwinner,pins", prop, name) {
		(*gpio_list)[i].name = name;

		if (of_property_read_u32(np_cfg, "allwinner,muxsel", &val)) {
			pr_warn("missing allwinner,mux property in node %s\n",
				np_cfg->name);
			return -EINVAL;
		}
		(*gpio_list)[i].mulsel = val;

		if (of_property_read_u32(np_cfg, "allwinner,drive", &val)) {
			pr_warn("missing allwinner,dirve property in node %s\n",
				np_cfg->name);
			return -EINVAL;
		}
		(*gpio_list)[i].drive = val;

		if (of_property_read_u32(np_cfg, "allwinner,pull", &val)) {
			pr_warn("missing allwinner,pull property in node %s\n",
				np_cfg->name);
			return -EINVAL;
		}
		(*gpio_list)[i].pull = val;

		if (of_property_read_u32(np_cfg, "allwinner,data", &val)) {
			pr_warn("missing allwinner,data property in node %s\n",
				np_cfg->name);
			return -EINVAL;
		}
		(*gpio_list)[i].data= val;

		i++;
	}

	return 0;
}

static int dt_get_gpio_list(struct device_node *np,
			   struct sunxi_gpio_config **gpio_list,
			   unsigned *gpio_count)
{
	struct property *prop;
	int size;
	const __be32 *list;
	struct device_node *np_cfg;
	phandle phandle;
	int ret;
	int i;

	/* only need pin whose state is default */
	prop = of_find_property(np, "pinctrl-0", &size);
	if (!prop) {
		pr_warn("missing pinctrl-0 property in node %s\n", np->name);
		return -EINVAL;
	}
	list = prop->value;
	size /= sizeof(*list);

	/* For every referenced pin configuration node in it */
	for (i= 0; i < size; i++) {
		phandle = be32_to_cpup(list++);

		/* Look up the pin configuration node */
		np_cfg = of_find_node_by_phandle(phandle);
		if (!np_cfg) {
			pr_warn("prop %s index 0 invalid phandle\n", prop->name);
			return -EINVAL;
		}

		/* Parse the node */
		ret = dt_node_to_gpio(np_cfg, gpio_list, gpio_count);
		if (ret < 0)
			return -EINVAL;
	}

	return 0;
}

static irqreturn_t sunxi_pinctrl_irq_handler_demo1(int irq, void *dev_id)
{
	pr_warn("-----------------------------------------------\n");
	pr_warn("%s: demo1 for test pinctrl repeat eint api.\n", __func__);
	pr_warn("-----------------------------------------------\n");
	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}

static irqreturn_t sunxi_pinctrl_irq_handler_demo2(int irq, void *dev_id)
{
	pr_warn("-----------------------------------------------\n");
	pr_warn("%s: demo2 for test pinctrl repeat eint api.\n", __func__);
	pr_warn("-----------------------------------------------\n");
	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}

static int pctrltest_request_all_resource(void)
{

	struct device *dev;
	struct device_node *node;
	struct pinctrl *pinctrl;
	struct sunxi_gpio_config *gpio_list = NULL;
	struct sunxi_gpio_config *gpio_cfg;
	unsigned gpio_count = 0;
	unsigned gpio_index;
	long unsigned int config;
	int ret;

	dev = bus_find_device_by_name(&platform_bus_type, NULL, sunxi_ptest_data->dev_name);
	if (!dev) {
		pr_warn("find device [%s] failed...\n", sunxi_ptest_data->dev_name);
		return -EINVAL;
	}

	node = of_find_node_by_type(NULL, dev_name(dev));
	if (!node) {
		pr_warn("find node for device [%s] failed...\n", dev_name(dev));
		return -EINVAL;
	}
	dev->of_node = node;

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("device[%s] all pin resource we want to request\n", dev_name(dev));
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: request pin all resource.\n");
	pinctrl = devm_pinctrl_get_select_default(dev);
	if (IS_ERR_OR_NULL(pinctrl)) {
		pr_warn("request pinctrl handle for device [%s] failed...\n", dev_name(dev));
		return -EINVAL;
	}

	pr_warn("step2: get device[%s] pin count.\n", dev_name(dev));
	ret = dt_get_gpio_list(node, &gpio_list, &gpio_count);
	if (ret < 0 || gpio_count == 0) {
		pr_warn(" devices own 0 pin resource or look for main key failed!\n");
		return -EINVAL;
	}

	pr_warn("step3: get device[%s] pin configure and check.\n", dev_name(dev));
	for (gpio_index=0; gpio_index<gpio_count; gpio_index++) {
		gpio_cfg = &gpio_list[gpio_index];

		/*check function config */
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
		pin_config_get(SUNXI_PINCTRL, gpio_cfg->name, &config);
		if (gpio_cfg->mulsel != SUNXI_PINCFG_UNPACK_VALUE(config)) {
			pr_warn("failed! mul value isn't equal as dt.\n");
			return -EINVAL;
		}

		/*check pull config */
		if (gpio_cfg->pull != GPIO_PULL_DEFAULT){
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0xFFFF);
			pin_config_get(SUNXI_PINCTRL, gpio_cfg->name, &config);
			if (gpio_cfg->pull != SUNXI_PINCFG_UNPACK_VALUE(config)) {
				pr_warn("failed! pull value isn't equal as dt.\n");
				return -EINVAL;
			}
		}

		/*check dlevel config */
		if (gpio_cfg->drive != GPIO_DRVLVL_DEFAULT){
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 0XFFFF);
			pin_config_get(SUNXI_PINCTRL, gpio_cfg->name, &config);
			if (gpio_cfg->drive != SUNXI_PINCFG_UNPACK_VALUE(config)) {
				pr_warn("failed! dlevel value isn't equal as dt.\n");
				return -EINVAL;
			}
		}

		/*check data config */
		if (gpio_cfg->data != GPIO_DATA_DEFAULT){
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, 0XFFFF);
			pin_config_get(SUNXI_PINCTRL, gpio_cfg->name, &config);
			if (gpio_cfg->data != SUNXI_PINCFG_UNPACK_VALUE(config)) {
				pr_warn("failed! pin data value isn't equal as dt.\n");
				return -EINVAL;
			}
		}
	}

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl request all resource success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_re_request_all_resource(void)
{
	struct device *dev;
	struct device_node *node;
	struct pinctrl *pinctrl_1;
	struct pinctrl *pinctrl_2;

	dev = bus_find_device_by_name(&platform_bus_type, NULL, sunxi_ptest_data->dev_name);
	if (!dev) {
		pr_warn("find device [%s] failed...\n", sunxi_ptest_data->dev_name);
		return -EINVAL;
	}

	node = of_find_node_by_type(NULL, dev_name(dev));
	if (!node) {
		pr_warn("find node for device [%s] failed...\n", dev_name(dev));
		return -EINVAL;
	}
	dev->of_node = node;

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("device[%s] all pin resource we want to repeat request\n", dev_name(dev));
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: first time request pin all resource.\n");
	/*request all resource */
	pinctrl_1 = devm_pinctrl_get_select_default(dev);
	if (IS_ERR_OR_NULL(pinctrl_1)) {
		pr_warn("request pinctrl handle for device [%s] failed!\n", dev_name(dev));
		return -EINVAL;
	}

	/*repeat request */
	pr_warn("step2: secondary request pin all resource.\n");
	pinctrl_2 = devm_pinctrl_get_select_default(dev);
	if (IS_ERR_OR_NULL(pinctrl_2)) {
		pr_warn("repeat request device[%s] all pin resource failed\n", dev_name(dev));
		return -EINVAL;
	}

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl repeat request all resource success.\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_request_gpio(void)
{
	int req_status;
	int gpio_index;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");
	pinctrl_free_gpio(gpio_index);

	/* request signal pin as gpio*/
	pr_warn("step1: pinctrl request gpio[%s]\n", pin_name);
	req_status = pinctrl_request_gpio(gpio_index);
	if (0 != req_status) {
		pr_warn("pinctrl request gpio failed! return value %d\n", req_status);
		return -EINVAL;
	}
	pr_warn("       pinctrl request gpio[%s] success\n", pin_name);
	pinctrl_free_gpio(gpio_index);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl request gpio api success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_free_gpio(void)
{
	int req_status;
	int gpio_index;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");
	pinctrl_free_gpio(gpio_index);

	/*request signal pin as gpio*/
	pr_warn("step1: pinctrl request gpio[%s]\n", pin_name);
	req_status = pinctrl_request_gpio(gpio_index);
	if (0 != req_status) {
		pr_warn("pinctrl request gpio failed !return value %d\n", req_status);
		return -EINVAL;
	}
	pr_warn("       pinctrl request gpio[%s]success\n", pin_name);

	pr_warn("step2: pinctrl free gpio[%s]\n", pin_name);
	pinctrl_free_gpio(gpio_index);

	pr_warn("step3: pinctrl request the same gpio[%s] again..\n", pin_name);
	req_status = pinctrl_request_gpio(gpio_index);
	if (0 != req_status) {
		pr_warn("pinctrl request gpio failed !return value %d\n", req_status);
		return -EINVAL;
	}
	pr_warn("       pinctrl request gpio[%s] again success.\n", pin_name);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl free gpio api success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	pinctrl_free_gpio(gpio_index);
	return 0;
}

static int pctrltest_lookup_state(void)
{
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *state;
	char device_name[SUNXI_DEV_NAME_MAX_LEN];

	dev = bus_find_device_by_name(&platform_bus_type, NULL, sunxi_ptest_data->dev_name);
	if (!dev) {
		pr_warn("find device [%s] failed...\n", sunxi_ptest_data->dev_name);
		return -EINVAL;
	}

	dev_set_name(dev, sunxi_ptest_data->dev_name);
	strcpy(device_name, sunxi_ptest_data->dev_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("device[%s] test lookup state api\n", dev_name(dev));
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: get pinctrl handle.\n");
	pinctrl = pinctrl_get(dev);
	if (IS_ERR_OR_NULL(pinctrl)) {
		pr_warn("get pinctrl handle [%s] failed...,return value %ld\n",
			device_name, PTR_ERR(pinctrl));
		return -EINVAL;
	}

	pr_warn("step2: printk pinctrl current state.\n");
	pr_warn("       device: %s current state: %s\n", dev_name(pinctrl->dev),
		pinctrl->state ? pinctrl->state->name : "none");

	pr_warn("step3: pinctrl lookup state(default state name: default).\n");
	state = pinctrl_lookup_state(pinctrl, "default");
	if (IS_ERR(state)) {
		pr_warn("can not find state: default.\n");
		return -EINVAL;
	}

	pr_warn("step4: check the state we lookup if the one needed.\n");
	if (strcmp(state->name, "default")) {
		pr_warn("find state,but isn't the one we need.\n");
		return -EINVAL;
	}
	pinctrl_put(pinctrl);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl look up state api success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_select_state(void)
{
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *state;
	char device_name[SUNXI_DEV_NAME_MAX_LEN];
	int req_status;

	dev = bus_find_device_by_name(&platform_bus_type, NULL, sunxi_ptest_data->dev_name);
	if (!dev) {
		pr_warn("find device [%s] failed...\n", sunxi_ptest_data->dev_name);
		return -EINVAL;
	}

	dev_set_name(dev, sunxi_ptest_data->dev_name);
	strcpy(device_name, sunxi_ptest_data->dev_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("device[%s] test select state api\n", dev_name(dev));
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: get pinctrl handle.\n");
	pinctrl = pinctrl_get(dev);
	if (IS_ERR_OR_NULL(pinctrl)) {
		pr_warn("get pinctrl handle [%s] failed..., return value %ld\n",
			device_name, PTR_ERR(pinctrl));
		return -EINVAL;
	}

	pr_warn("step2: printk pinctrl current state.\n");
	pr_warn("       device: %s current state: %s\n", dev_name(pinctrl->dev),
		pinctrl->state ? pinctrl->state->name : "none");

	pr_warn("step3: pinctrl lookup state(default state name: default).\n");
	state = pinctrl_lookup_state(pinctrl, "default");
	if (IS_ERR(state)) {
		pinctrl_put(pinctrl);
		pr_warn("can not find state: default.\n");
		return -EINVAL;
	}

	pr_warn("step4: check the state we lookup if the one needed.\n");
	if (strcmp(state->name, "default")) {
		pinctrl_put(pinctrl);
		pr_warn("find state,but isn't the one we need.\n");
		return -EINVAL;
	}

	pr_warn("step5: select state for pinctrl handle.\n");
	req_status = pinctrl_select_state(pinctrl, state);
	if (req_status < 0) {
		pinctrl_put(pinctrl);
		pr_warn("pinctrl select state failed. return value %d.\n",req_status);
	}
	pinctrl_put(pinctrl);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl select state api success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_get(void)
{
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *state;
	struct pinctrl_setting *setting;
	char device_name[SUNXI_DEV_NAME_MAX_LEN];

	dev = bus_find_device_by_name(&platform_bus_type, NULL, sunxi_ptest_data->dev_name);
	if (!dev) {
		pr_warn("find device [%s] failed...\n", sunxi_ptest_data->dev_name);
		return -EINVAL;
	}

	dev_set_name(dev, sunxi_ptest_data->dev_name);
	strcpy(device_name, sunxi_ptest_data->dev_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("device[%s] test pinctrl get api\n", dev_name(dev));
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: get pinctrl handle.\n");
	pinctrl = pinctrl_get(dev);
	if (IS_ERR_OR_NULL(pinctrl)) {
		pr_warn("get pinctrl handle [%s] failed..., return value %ld\n",
			dev_name(dev), PTR_ERR(pinctrl));
		return -EINVAL;
	}

	pr_warn("step2: check pinctrl handle we have getted.\n");
	if (dev_name(dev) != dev_name(pinctrl->dev)) {
		pinctrl_put(pinctrl);
		pr_warn("check: pinctrl handle isn't that one we want\n ");
		return -EINVAL;
	}

	pr_warn("step3: get current statep.\n");
	pr_warn("       device: %s current state: %s\n", dev_name(pinctrl->dev),
		 pinctrl->state ? pinctrl->state->name : "none");
	list_for_each_entry(state, &pinctrl->states, node) {
		pr_warn("state: %s\n", state->name);
		list_for_each_entry(setting, &state->settings, node) {
			struct pinctrl_dev *pctldev = setting->pctldev;
			pr_warn("      setting type: %d   pin controller %s \n",
				setting->type, pinctrl_dev_get_name(pctldev));
		}
	}
	pinctrl_put(pinctrl);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl get api success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_put(void)
{
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *state;
	struct pinctrl_setting *setting;
	char device_name[SUNXI_DEV_NAME_MAX_LEN];

	dev = bus_find_device_by_name(&platform_bus_type, NULL, sunxi_ptest_data->dev_name);
	if (!dev) {
		pr_warn("find device [%s] failed...\n", sunxi_ptest_data->dev_name);
		return -EINVAL;
	}

	dev_set_name(dev, sunxi_ptest_data->dev_name);
	strcpy(device_name, sunxi_ptest_data->dev_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("device[%s] test pinctrl put api\n", dev_name(dev));
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: get pinctrl handle.\n");
	pinctrl = pinctrl_get(dev);
	if (IS_ERR_OR_NULL(pinctrl)) {
		pr_warn("get pinctrl handle [%s] failed...,return value %ld\n",
			device_name, PTR_ERR(pinctrl));
		return -EINVAL;
	}

	pr_warn("step2: check pinctrl handle we have getted.\n");
	if (dev_name(dev) != dev_name(pinctrl->dev)) {
		pinctrl_put(pinctrl);
		pr_warn("check: pinctrl handle isn't that one we want\n");
		return -EINVAL;
	}

	pr_warn("step3: get current statep.\n");
	pr_warn("       device: %s current state: %s\n", dev_name(pinctrl->dev),
		pinctrl->state ? pinctrl->state->name : "none");
	list_for_each_entry(state, &pinctrl->states, node) {
		pr_warn("state: %s\n", state->name);
		list_for_each_entry(setting, &state->settings, node) {
			struct pinctrl_dev *pctldev = setting->pctldev;
			pr_warn("    setting type: %d   pin controller %s \n",
				setting->type, pinctrl_dev_get_name(pctldev));
		}
	}

	pr_warn("step4: free pinctrl handle we have getted.\n");
	pinctrl_put(pinctrl);

	pr_warn("step5: then repeat get. if get success, previous free operate success.\n");
	pinctrl = pinctrl_get(dev);
	if (IS_ERR_OR_NULL(pinctrl)) {
		pr_warn("       after free, we repeat get pinctrl handle [%s] failed..., return value %ld\n",
			device_name, PTR_ERR(pinctrl));
		return -EINVAL;
	}
	pinctrl_put(pinctrl);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl put api success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_devm_get_and_put(void)
{
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *state;
	struct pinctrl_setting *setting;
	char device_name[SUNXI_DEV_NAME_MAX_LEN];

	dev = bus_find_device_by_name(&platform_bus_type, NULL, sunxi_ptest_data->dev_name);
	if (!dev) {
		pr_warn("find device [%s] failed...\n", sunxi_ptest_data->dev_name);
		return -EINVAL;
	}

	dev_set_name(dev, sunxi_ptest_data->dev_name);
	strcpy(device_name, sunxi_ptest_data->dev_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("device[%s] test pinctrl devm get and put api\n", dev_name(dev));
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: devm get pinctrl handle.\n");
	pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pinctrl)) {
		pr_warn("get pinctrl handle [%s] failed..., return value %ld\n",
			device_name, PTR_ERR(pinctrl));
		return -EINVAL;
	}

	pr_warn("step2: check pinctrl handle we have getted.\n");
	if (dev_name(dev) != dev_name(pinctrl->dev)) {
		devm_pinctrl_put(pinctrl);
		pr_warn("check: pinctrl handle isn't that one we want\n ");
		return -EINVAL;
	}

	pr_warn("step3: get current statep.\n");
	pr_warn("       device: %s current state: %s\n", dev_name(pinctrl->dev),
		pinctrl->state ? pinctrl->state->name : "none");
	list_for_each_entry(state, &pinctrl->states, node) {
		pr_warn("state: %s\n", state->name);
		list_for_each_entry(setting, &state->settings, node) {
			struct pinctrl_dev *pctldev = setting->pctldev;
			pr_warn("      setting type: %d   pin controller %s \n",
				setting->type, pinctrl_dev_get_name(pctldev));
		}
	}

	pr_warn("step4: devm free pinctrl handle we have getted.\n");
	devm_pinctrl_put(pinctrl);

	pr_warn("step5: then repeat get. if get success, previous free operate success.\n");
	pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pinctrl)) {
		pr_warn("       after free,we repeat get pinctrl handle [%s] failed..., return value %ld\n",
			device_name, PTR_ERR(pinctrl));
		return -EINVAL;
	}
	devm_pinctrl_put(pinctrl);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl devm get and put api success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_function_set(void)
{
	long unsigned int config_set;
	long unsigned int config_get;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	int func;

	func = sunxi_ptest_data->funcs;
	sunxi_gpio_to_name(sunxi_ptest_data->gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("pin function we want to set:\n");
	pr_warn(" gpio name: %s	    gpio index: %d       gpio function: %d\n",
		pin_name, sunxi_ptest_data->gpio_index, func);
	pr_warn("-----------------------------------------------\n");

	/*check if pin mul setting right */
	pr_warn("step1: get [%s] function value.\n", pin_name);
	config_get = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0XFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config_get);
	pr_warn("       [%s] function value: %ld\n", pin_name, SUNXI_PINCFG_UNPACK_VALUE(config_get));

	pr_warn("step2: set [%s] function value to %d\n", pin_name, func);
	config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, func);
	pin_config_set(SUNXI_PINCTRL, pin_name, config_set);

	pr_warn("step3: get [%s] function value and check.\n", pin_name);
	config_get = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0XFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config_get);
	if (func != SUNXI_PINCFG_UNPACK_VALUE(config_get)){
		pr_warn("test pin config for mul setting failed !\n");
		return -EINVAL;
	}

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl function set success ! \n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_data_set(void)
{
	long unsigned int config_set;
	long unsigned int config_get;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	int data;

	data = sunxi_ptest_data->data;
	sunxi_gpio_to_name(sunxi_ptest_data->gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("pin data we want to set:\n");
	pr_warn(" gpio name: %s	    gpio index: %d       gpio data: %d\n",
		pin_name, sunxi_ptest_data->gpio_index, data);
	pr_warn("-----------------------------------------------\n");

	/*check if pin data setting right */
	pr_warn("step1: get [%s] data value.\n", pin_name);
	config_get = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, 0XFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config_get);
	pr_warn("       [%s] data value: %ld\n", pin_name, SUNXI_PINCFG_UNPACK_VALUE(config_get));

	pr_warn("step2: set [%s] data value to %d\n", pin_name, data);
	config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, data);
	pin_config_set(SUNXI_PINCTRL, pin_name, config_set);

	pr_warn("step3: get [%s] data value and check.\n", pin_name);
	config_get = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, 0XFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config_get);
	if (data != SUNXI_PINCFG_UNPACK_VALUE(config_get)) {
		pr_warn("test pin config for dlevel setting failed !\n");
		return -EINVAL;
	}

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl data set success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_pull_set(void)
{
	long unsigned int config_set;
	long unsigned int config_get;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	int pull;

	pull = sunxi_ptest_data->pull;
	sunxi_gpio_to_name(sunxi_ptest_data->gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("pin data we want to set:\n");
	pr_warn(" gpio name: %s	    gpio index: %d       gpio pull: %d\n",
		pin_name, sunxi_ptest_data->gpio_index, pull);
	pr_warn("-----------------------------------------------\n");

	/*check if pin pull setting right */
	pr_warn("step1: get [%s] pull value.\n", pin_name);
	config_get = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0XFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config_get);
	pr_warn("       [%s] pull value: %ld\n", pin_name, SUNXI_PINCFG_UNPACK_VALUE(config_get));

	pr_warn("step2: set [%s] pull value to %d\n", pin_name, pull);
	config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pull);
	pin_config_set(SUNXI_PINCTRL, pin_name, config_set);

	pr_warn("step3: get [%s] function value and check.\n", pin_name);
	config_get = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0XFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config_get);
	if (pull != SUNXI_PINCFG_UNPACK_VALUE(config_get)) {
		pr_warn("test pin config for pull setting failed !\n");
		return -EINVAL;
	}

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl pull set success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_dlevel_set(void)
{
	long unsigned int config_set;
	long unsigned int config_get;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	int dlevel;

	dlevel = sunxi_ptest_data->dlevel;
	sunxi_gpio_to_name(sunxi_ptest_data->gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("pin data we want to set:\n");
	pr_warn(" gpio name: %s	    gpio index: %d       gpio dlevel: %d\n",
		pin_name, sunxi_ptest_data->gpio_index, dlevel);
	pr_warn("-----------------------------------------------\n");

	/*check if pin dlevel setting right */
	pr_warn("step1: get [%s] dlevel value.\n", pin_name);
	config_get = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 0XFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config_get);
	pr_warn("       [%s] dlevel value: %ld\n", pin_name, SUNXI_PINCFG_UNPACK_VALUE(config_get));

	pr_warn("step2: set [%s] dlevel value to %d\n", pin_name, dlevel);
	config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, dlevel);
	pin_config_set(SUNXI_PINCTRL, pin_name, config_set);

	pr_warn("step3: get [%s] dlevel value and check.\n", pin_name);
	config_get = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 0XFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config_get);
	if (dlevel != SUNXI_PINCFG_UNPACK_VALUE(config_get)) {
		pr_warn("test pin config for dlevel setting failed !\n");
		return -EINVAL;
	}

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pinctrl drive level set success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_direction_input(void)
{
	int gpio_index;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	int req_status;
	int direct_status;
	long unsigned int config;

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(sunxi_ptest_data->gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n",pin_name, gpio_index);
	pinctrl_free_gpio(gpio_index);
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: Pinctrl request gpio.\n");
	req_status = pinctrl_request_gpio(gpio_index);
	if(0 != req_status){
		pr_warn("pinctrl request gpio failed !return value %d\n", req_status);
		return -EINVAL;
	}

	pr_warn("step2: Set gpio direction input.\n");
	direct_status = pinctrl_gpio_direction_input(gpio_index);
	if (IS_ERR_VALUE(direct_status)) {
		pr_warn("set pinctrl gpio direction input failed! return value: %d\n",
			direct_status);
		return -EINVAL;
	}

	pr_warn("step3: Get pin mux value and check.\n");
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config);
	if (0 != SUNXI_PINCFG_UNPACK_VALUE(config)){
		pr_warn("check: set pin direction input failed !\n");
		return -EINVAL;
	}

	pr_warn("step4: Pinctrl free gpio.\n");
	pinctrl_free_gpio(gpio_index);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test gpio direction input success!\n");
	pr_warn("+++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_direction_output(void)
{
	int gpio_index;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	int req_status;
	int direct_status;
	long unsigned int config;

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(sunxi_ptest_data->gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");
	pinctrl_free_gpio(gpio_index);

	pr_warn("step1: Pinctrl request gpio.\n");
	req_status = pinctrl_request_gpio(gpio_index);
	if(0 != req_status){
		pr_warn("pinctrl request gpio failed !return value %d\n",req_status);
		return -EINVAL;
	}

	pr_warn("step2: Set gpio direction output.\n");
	direct_status = pinctrl_gpio_direction_output(gpio_index);
	if (IS_ERR_VALUE(direct_status)) {
		pr_warn("set pinctrl gpio direction output failed! return value: %d\n",
			direct_status);
		return -EINVAL;
	}

	pr_warn("step3: Get pin mux value and check.\n");
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config);
	if (1 != SUNXI_PINCFG_UNPACK_VALUE(config)){
		pr_warn("check: set pinctrl gpio direction output failed !\n");
		return -EINVAL;
	}

	pr_warn("step4: Pinctrl free gpio.\n");
	pinctrl_free_gpio(gpio_index);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test gpio direction output success!\n");
	pr_warn("+++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int pctrltest_request_eint(void)
{
	int virq;
	int req_status;
	int set_direct_status;
	int req_IRQ_status;
	int gpio_index;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: request gpio [%s].\n", pin_name);
	req_status = gpio_request(gpio_index, NULL);
	if(0 != req_status){
		pr_warn("gpio request failed \n");
		return -EINVAL;
	}

	pr_warn("step2: set gpio[%s]direction output and data value 0.\n", pin_name);
	set_direct_status = gpio_direction_output(gpio_index, 0);
	if (IS_ERR_VALUE(set_direct_status)) {
		pr_warn("set gpio direction output failed for check gpio get value %d\n",
			set_direct_status);
		return -EINVAL;
	}
	gpio_free(gpio_index);

	pr_warn("step3: generate virtual irq number.\n");
	virq = gpio_to_irq(gpio_index);
	if (IS_ERR_VALUE(virq)) {
		pr_warn("map gpio [%d] to virq [%d] failed !\n ", gpio_index, virq);
		return -EINVAL;
	}

	pr_warn("step4: request irq(low level trigger).\n");
	req_IRQ_status = request_irq(virq, sunxi_pinctrl_irq_handler_demo1,
					IRQF_TRIGGER_LOW, "PIN_EINT", NULL);
	if (IS_ERR_VALUE(req_IRQ_status)) {
		pr_warn("request irq failed !\n");
		return -EINVAL;
	}
	free_irq(virq, NULL);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test pin eint sunccess !\n");
	pr_warn("+++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n\n");
	return 0;
}

static int pctrltest_re_request_eint(void)
{
	int virq;
	int req_status;
	int set_direct_status;
	int req_IRQ_status;
	int re_req_IRQ_status;
	int gpio_index;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: request gpio [%s].\n", pin_name);
	req_status = gpio_request(gpio_index, NULL);
	if (0 != req_status) {
		pr_warn("gpio request failed \n");
		return -EINVAL;
	}

	pr_warn("step2: set gpio[%s]direction output and data value 0.\n", pin_name);
	set_direct_status = gpio_direction_output(gpio_index, 0);
	if (IS_ERR_VALUE(set_direct_status)) {
		pr_warn("set gpio direction output failed for check gpio get value %d\n",
			set_direct_status);
		return -EINVAL;
	}
	gpio_free(gpio_index);

	pr_warn("step3: generate virtual irq number.\n");
	virq = gpio_to_irq(gpio_index);
	if (IS_ERR_VALUE(virq)) {
		pr_warn("map gpio [%d] to virq [%d] failed !\n ", gpio_index, virq);
		return -EINVAL;
	}

	pr_warn("step4: first time request irq(low level trigger).\n");
	req_IRQ_status = request_irq(virq, sunxi_pinctrl_irq_handler_demo1,
				    IRQF_TRIGGER_LOW, "PIN_EINT", NULL);
	if (IS_ERR_VALUE(req_IRQ_status)) {
		free_irq(virq, NULL);
		pr_warn("test pin request irq failed !\n");
		return -EINVAL;
	}

	pr_warn("step5: repeat request irq(low level trigger).\n");
	re_req_IRQ_status = request_irq(virq, sunxi_pinctrl_irq_handler_demo2,
					IRQF_TRIGGER_LOW, "PIN_EINT", NULL);
	free_irq(virq, NULL);
	if (!IS_ERR_VALUE(re_req_IRQ_status)) {
		pr_warn("      repeat request irq success!\n\n");
		pr_warn("test failed! for repeat request is umpermitted.\n");
		return -EINVAL;
	}

	pr_warn("      repeat request irq failed!\n");
	pr_warn("test sunccess! for repeat request is umpermitted.\n");

	pr_warn("-----------------------------------------------\n");
	pr_warn("test picntrl repeat eint success!\n");
	pr_warn("+++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int gpiotest_request_free(void)
{
	int gpio_index;
	int req_status;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: request gpio[%s]\n", pin_name);
	gpio_free(gpio_index);
	req_status = gpio_request(gpio_index, NULL);
	if(0 != req_status){
		pr_warn("gpio request failed !return value %d\n", req_status);
		return -EINVAL;
	}
	pr_warn("       request gpio[%s] success\n", pin_name);

	pr_warn("step2: free gpio[%s]\n", pin_name);
	gpio_free(gpio_index);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test gpio request and free success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int gpiotest_re_request_free(void)
{
	int gpio_index;
	int req_status;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(sunxi_ptest_data->gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");

	pr_warn("step1: first time request gpio[%s]\n", pin_name);
	req_status = gpio_request(gpio_index, NULL);
	if(0 != req_status){
		pr_warn("      first time request gpio [%s]failed !\n", pin_name);
		return -EINVAL;
	}
	pr_warn("       first time request gpio[%s] success!\n", pin_name);

	pr_warn("step2: repeat request gpio[%s]\n", pin_name);
	req_status = gpio_request(gpio_index, NULL);
	if (!req_status) {
		pr_warn("      repeat request gpio[%s] success.\n", pin_name);
		pr_warn("test failed: for repeat request is unpermitted.\n");
		return -EINVAL;
	}

	pr_warn("       repeat request gpio [%s] failed.\n", pin_name);
	pr_warn("test success: for repeat request is unpermitted.\n");

	pr_warn("-----------------------------------------------\n");
	pr_warn("test gpio repeat request and free success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	gpio_free(gpio_index);
	return 0;
}

static int gpiotest_set_debounce(void)
{
	int gpio_index;
	int req_status;
	int get_status;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");

	/*
	 * test gpio set debounce api
	 */
	pr_warn("step1: request gpio.\n");
	req_status = gpio_request(gpio_index,NULL);
	if(0 != req_status){
		pr_warn("gpio request failed !\n");
		return -EINVAL;
	}

	pr_warn("step2: set gpio debounce value 0x11.\n");
	get_status = gpio_set_debounce(gpio_index,0x11);
	if(get_status){
		pr_warn("      gpio set debounce failed! return value: %d\n",get_status);
		gpio_free(gpio_index);
		return -EINVAL;
	}

	pr_warn("step3: gpio free.\n");
	gpio_free(gpio_index);

	pr_warn("-----------------------------------------------\n");
	pr_warn("test gpio set debounce success!\n");
	pr_warn("++++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;
}

static int gpiotest_gpiolib(void)
{
	int gpio_index;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	int req_status;
	int set_direct_status;
	long unsigned int config;
	int val;

	gpio_index = sunxi_ptest_data->gpio_index;
	sunxi_gpio_to_name(gpio_index, pin_name);

	pr_warn("++++++++++++++++++++++++++++%s++++++++++++++++++++++++++++\n", __func__);
	pr_warn("gpio name is : %s	gpio index is : %d\n", pin_name, gpio_index);
	pr_warn("-----------------------------------------------\n");

	/*
	 * test gpio set direction input api
	 */
	pr_warn("-----------------------------------------------\n");
	pr_warn("1. test gpio direction input api:\n");
	pr_warn("step1: request gpio.\n");
	req_status = gpio_request(gpio_index, NULL);
	if(0 != req_status){
		pr_warn("gpio request failed !\n");
		return -EINVAL;
	}

	pr_warn("step2: set gpio direction input.\n");
	set_direct_status = gpio_direction_input(gpio_index);
	if (IS_ERR_VALUE(set_direct_status)) {
		pr_warn("set gpio direction input failed!\n");
		goto test_gpiolib_api_failed;
	}

	pr_warn("step3: get gpio mux value and check.\n");
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config);
	if (0 != SUNXI_PINCFG_UNPACK_VALUE(config)){
		pr_warn("test gpio set direction input failed !\n");
		goto test_gpiolib_api_failed;
	}

	gpio_free(gpio_index);
	pr_warn("step4: gpio free.\n");
	pr_warn("finish API(gpio_direction_input)testing.\n");
	pr_warn("-----------------------------------------------\n\n");

	/*
	 * test gpio set direction output api
	 */
	pr_warn("2. test gpio direction output api:\n");
	pr_warn("step1: request gpio.\n");
	req_status = gpio_request(gpio_index, NULL);
	if(0 != req_status){
		pr_warn("gpio request failed!\n");
		return -EINVAL;
	}

	pr_warn("step2: set gpio direction output(data value 1).\n");
	set_direct_status = gpio_direction_output(gpio_index, 1);
	if (IS_ERR_VALUE(set_direct_status)) {
		pr_warn("set gpio direction output failed! \n");
		goto test_gpiolib_api_failed;
	}

	pr_warn("step3: get gpio mux value and check.\n");
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config);
	if (1 != SUNXI_PINCFG_UNPACK_VALUE(config)){
		pr_warn("faile!FUNC value not the same as expectation.\n");
		goto test_gpiolib_api_failed;
	}

	pr_warn("step4: get gpio data value and check.\n");
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config);
	if (1 != SUNXI_PINCFG_UNPACK_VALUE(config)){
		pr_warn("failed!DATA value not the same as expectation(1).\n");
		goto test_gpiolib_api_failed;
	}

	pr_warn("step5: set gpio direction output(data value 0).\n");
	set_direct_status = gpio_direction_output(gpio_index, 0);
	if (IS_ERR_VALUE(set_direct_status)) {
		pr_warn("set gpio direction output failed!\n");
		goto test_gpiolib_api_failed;
	}

	pr_warn("step6: get gpio data value and check.\n");
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config);
	if (0 != SUNXI_PINCFG_UNPACK_VALUE(config)){
		pr_warn("failed!DATA value not the same as expectation(0).\n");
		goto test_gpiolib_api_failed;
	}

	gpio_free(gpio_index);
	pr_warn("step7: gpio free.\n");
	pr_warn("finish API(gpio_direction_output)testing.\n");
	pr_warn("-----------------------------------------------\n\n");

	/*
	 * test gpio get value api
	 */
	pr_warn("3. test gpio get value api:\n");
	pr_warn("step1: request gpio.\n");
	req_status = gpio_request(gpio_index, NULL);
	if(0 != req_status){
		pr_warn("gpio request failed !\n");
		return -EINVAL;
	}

	pr_warn("step2: set gpio direction output(data value 0).\n");
	set_direct_status = gpio_direction_output(gpio_index, 0);
	if (IS_ERR_VALUE(set_direct_status)) {
		pr_warn("set gpio direction output failed !\n");
		goto test_gpiolib_api_failed;
	}
	pr_warn("step3: get gpio data value and check.\n");
	val=__gpio_get_value(gpio_index);
	pr_warn("       gpio data value :    %d \n",val);
	if (0 != val){
		pr_warn("failed!DATA value not the same as expectation.\n");
		goto test_gpiolib_api_failed;
	}

	gpio_free(gpio_index);
	pr_warn("step4: gpio free.\n");
	pr_warn("finish API(gpio_get_value)testing.\n");
	pr_warn("-----------------------------------------------\n\n");

	/*
	 * test gpio set value api
	 */
	pr_warn("4. test gpio set value api:\n");
	pr_warn("step1: request gpio.\n");
	req_status = gpio_request(gpio_index,NULL);
	if(0 != req_status){
		pr_warn("gpio request failed!\n");
		return -EINVAL;
	}

	pr_warn("step2: set gpio direction output(set data value 0).\n");
	set_direct_status = gpio_direction_output(gpio_index, 0);
	if (IS_ERR_VALUE(set_direct_status)) {
		pr_warn("set gpio direction output failed \n");
		goto test_gpiolib_api_failed;
	}

	pr_warn("step3: get gpio data value,then set 1 and check.\n");
	val=__gpio_get_value(gpio_index);
	pr_warn("       get gpio data value :    %d \n",val);
	__gpio_set_value(gpio_index, 1);
	pr_warn("       set gpio data value :    1 \n");
	val=__gpio_get_value(gpio_index);
	pr_warn("       get gpio data value :    %d \n",val);
	if (1 != val){
		pr_warn("test gpio set dat value 1 failed ! \n");
		goto test_gpiolib_api_failed;
	}

	pr_warn("step4: get gpio data value,then set 0 and check.\n");
	val=__gpio_get_value(gpio_index);
	pr_warn("       get gpio data value :    %d \n",val);
	__gpio_set_value(gpio_index, 0);
	pr_warn("       set gpio data value :    0 \n");
	val=__gpio_get_value(gpio_index);
	pr_warn("       get gpio data value :    %d \n",val);
	if (0 != val){
		pr_warn("test gpio set dat value 0 failed ! \n");
		goto test_gpiolib_api_failed;
	}

	gpio_free(gpio_index);
	pr_warn("step5: gpio free.\n");
	pr_warn("finish API(gpio_set_value)testing.\n");

	pr_warn("-----------------------------------------------\n");
	pr_warn("test gpio gpiolib success!\n");
	pr_warn("+++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	return 0;

test_gpiolib_api_failed:
	pr_warn("-----------------------------------------------\n");
	pr_warn("test gpio gpiolib failed!\n");
	pr_warn("+++++++++++++++++++++++++++end++++++++++++++++++++++++++++\n\n");
	gpio_free(gpio_index);
	return -EINVAL;
}

static struct sunxi_pctrltest_case sunxi_ptest_case[] = {
	{"pctrltest_request_all_resource", pctrltest_request_all_resource},
	{"pctrltest_re_request_all_resource", pctrltest_re_request_all_resource},
	{"pctrltest_request_gpio", pctrltest_request_gpio},
	{"pctrltest_free_gpio", pctrltest_free_gpio},
	{"pctrltest_lookup_state", pctrltest_lookup_state},
	{"pctrltest_select_state", pctrltest_select_state},
	{"pctrltest_get", pctrltest_get},
	{"pctrltest_put", pctrltest_put},
	{"pctrltest_devm_get_and_put", pctrltest_devm_get_and_put},
	{"pctrltest_function_set", pctrltest_function_set},
	{"pctrltest_data_set", pctrltest_data_set},
	{"pctrltest_pull_set", pctrltest_pull_set},
	{"pctrltest_dlevel_set", pctrltest_dlevel_set},
	{"pctrltest_direction_input", pctrltest_direction_input},
	{"pctrltest_direction_output", pctrltest_direction_output},
	{"pctrltest_request_eint", pctrltest_request_eint},
	{"pctrltest_re_request_eint", pctrltest_re_request_eint},
	{"gpiotest_request_free", gpiotest_request_free},
	{"gpiotest_re_request_free", gpiotest_re_request_free},
	{"gpiotest_set_debounce", gpiotest_set_debounce},
	{"gpiotest_gpiolib", gpiotest_gpiolib},
};

static ssize_t show_funcs(struct class *class, struct class_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%u\n", sunxi_ptest_data->funcs);
}

static ssize_t show_data(struct class *class, struct class_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%u\n", sunxi_ptest_data->data);
}

static ssize_t show_pull(struct class *class, struct class_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%u\n", sunxi_ptest_data->pull);
}

static ssize_t show_dlevel(struct class *class, struct class_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%u\n", sunxi_ptest_data->dlevel);
}

static ssize_t show_gpio_index(struct class *class, struct class_attribute *attr,
		char *buf)
{
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	sunxi_gpio_to_name(sunxi_ptest_data->gpio_index, pin_name);
	return sprintf(buf, "%s\n", pin_name);
}

static ssize_t show_dev_name(struct class *class,struct class_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%s\n", sunxi_ptest_data->dev_name);
}

static ssize_t show_result(struct class *class,struct class_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%d\n", sunxi_ptest_data->result);
}

static ssize_t show_exec(struct class *class, struct class_attribute *attr,
		char *buf)
{
	int i;
	int total_len = 0;
	struct sunxi_pctrltest_case *p = sunxi_ptest_case;

	for (i = 0; i < NUM_ELEMS(sunxi_ptest_case); i++, p++) {
		total_len += snprintf(buf+total_len, SUNXI_FUNC_NAME_MAX_LEN, "%s\n", p->name);
		if (total_len > PAGE_SIZE - SUNXI_FUNC_NAME_MAX_LEN) {
			pr_warn("can't show so many exec funcs.\n");
			return -EINVAL;
		}
	}

	return total_len;
}

static int get_parameter(const char *buf, int *data, size_t size)
{
	char *after;
	size_t count;
	int tmp;

	tmp = simple_strtoul(buf, &after, 10);
	count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size){
		*data = tmp;
		return size;
	}

	return -EINVAL;
}

static int get_exec_number(void)
{
	int i;
	struct sunxi_pctrltest_case *p = sunxi_ptest_case;

	for (i = 0; i < NUM_ELEMS(sunxi_ptest_case); i++, p++) {
		if (strcmp(p->name, sunxi_ptest_data->exec) == 0)
			return i;
	}

	return -1;
}

static ssize_t store_funcs(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	return get_parameter(buf, &sunxi_ptest_data->funcs, count);
}

static ssize_t store_data(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	return get_parameter(buf, &sunxi_ptest_data->data, count);
}

static ssize_t store_pull(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	return get_parameter(buf, &sunxi_ptest_data->pull, count);
}

static ssize_t store_dlevel(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	return get_parameter(buf, &sunxi_ptest_data->dlevel, count);
}

static ssize_t store_gpio_index(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	return get_parameter(buf, &sunxi_ptest_data->gpio_index, count);
}

static ssize_t store_dev_name(struct class *class, struct class_attribute *attr,
	       	const char *buf, size_t count)
{
	int ret;

	if (count > SUNXI_DEV_NAME_MAX_LEN) {
		pr_warn("sunxi dev name max len less than 20 char.\n");
		return -EINVAL;
	}
	ret = strlcpy(sunxi_ptest_data->dev_name, buf, count);

	return ret;
}

static ssize_t store_result(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	return get_parameter(buf, &sunxi_ptest_data->result, count);
}

static ssize_t store_exec(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int number;

	if (count > SUNXI_FUNC_NAME_MAX_LEN) {
		pr_warn("sunxi func name max len less than 80 char.\n");
		return -EINVAL;
	}
	ret = strlcpy(sunxi_ptest_data->exec, buf, count);

	number = get_exec_number();
	if (number < 0) {
		pr_warn("can't find exec number.\n");
		return -EINVAL;
	}

	sunxi_ptest_data->result = sunxi_ptest_case[number].func();

	return ret;
}

static struct class_attribute sunxi_pctrltest_attrs[] = {
	__ATTR(data, S_IRUGO | S_IWUSR, show_data, store_data),
	__ATTR(dlevel, S_IRUGO | S_IWUSR, show_dlevel, store_dlevel),
	__ATTR(funcs, S_IRUGO | S_IWUSR, show_funcs, store_funcs),
	__ATTR(pull, S_IRUGO | S_IWUSR, show_pull, store_pull),
	__ATTR(gpio_index, S_IRUGO | S_IWUSR, show_gpio_index, store_gpio_index),
	__ATTR(dev_name, S_IRUGO | S_IWUSR, show_dev_name, store_dev_name),
	__ATTR(exec, S_IRUGO | S_IWUSR, show_exec, store_exec),
	__ATTR(result, S_IRUGO, show_result, store_result),
	__ATTR_NULL,
};

static struct class sunxi_pctrltest_class = {
	.name		= "sunxi_pinctrl_test",
	.owner		= THIS_MODULE,
	.class_attrs	= sunxi_pctrltest_attrs,
};

static int sunxi_pctrltest_probe(struct platform_device *pdev)
{
	int ret;

	sunxi_ptest_data = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_pctrltest_data), GFP_KERNEL);
	if (!sunxi_ptest_data) {
		pr_err("no enougt memory for sunxi pinctrl test data\n");
		return -ENOMEM;
	}
	sunxi_ptest_data->result = -1;

	ret = class_register(&sunxi_pctrltest_class);
	if(ret < 0) {
		pr_err("register sunxi pinctrl test class failed: %d\n", ret);
		return -ENOMEM;
	}

	pdev->dev.class = &sunxi_pctrltest_class;
	dev_set_name(&pdev->dev, "Vdevice");
	platform_set_drvdata(pdev, sunxi_ptest_data);

	return ret;
}

static int sunxi_pctrltest_remove(struct platform_device *pdev)
{
	class_unregister(&sunxi_pctrltest_class);
	return 0;
}

static struct of_device_id sunxi_pctrltest_match[] = {
	{ .compatible = "allwinner,sun8i-vdevice"},
	{ .compatible = "allwinner,sun50i-vdevice"},
	{}
};

static struct platform_driver sunxi_pctrltest_driver = {
	.probe = sunxi_pctrltest_probe,
	.remove	= sunxi_pctrltest_remove,
	.driver = {
		.name = "Vdevice",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_pctrltest_match,
	},
};

static int __init sunxi_pctrltest_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_pctrltest_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("register sunxi pinctrl platform driver failed\n");
		return -EINVAL;
	}

	return 0;
}

static void __exit sunxi_pctrltest_exit(void)
{
	platform_driver_unregister(&sunxi_pctrltest_driver);
}

module_init(sunxi_pctrltest_init);
module_exit(sunxi_pctrltest_exit);
MODULE_AUTHOR("WimHuang<huangwei@allwinnertech.com");
MODULE_AUTHOR("Huangshr<huangshr@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner SUNXI Pinctrl driver test");
MODULE_LICENSE("GPL");
