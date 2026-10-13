/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/export.h>
#include <linux/gfp.h>

struct devm_clk_state {
	struct clk *clk;
};

static void devm_clk_release(struct device *dev, void *res)
{
	struct devm_clk_state *state = res;
	clk_put(state->clk);
}

struct clk *devm_clk_get(struct device *dev, const char *id)
{
	struct devm_clk_state *state, *clk;

	state = devres_alloc(devm_clk_release, sizeof(*state), GFP_KERNEL);
	if (!state)
		return ERR_PTR(-ENOMEM);

	clk = clk_get(dev, id);
	if (!IS_ERR(clk)) {
		state->clk = clk;
		devres_add(dev, state);
	} else {
		devres_free(state);
	}

	return clk;
}
EXPORT_SYMBOL(devm_clk_get);

struct clk_bulk_devres {
	struct clk_bulk_data *clks;
	int num_clks;
};

static void devm_clk_bulk_release(struct device *dev, void *res)
{
	struct clk_bulk_devres *devres = res;

	clk_bulk_put(devres->num_clks, devres->clks);
}

int __must_check devm_clk_bulk_get(struct device *dev, int num_clks,
		      struct clk_bulk_data *clks)
{
	struct clk_bulk_devres *devres;
	int ret;

	devres = devres_alloc(devm_clk_bulk_release,
			      sizeof(*devres), GFP_KERNEL);
	if (!devres)
		return -ENOMEM;

	ret = clk_bulk_get(dev, num_clks, clks);
	if (!ret) {
		devres->clks = clks;
		devres->num_clks = num_clks;
		devres_add(dev, devres);
	} else {
		devres_free(devres);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(devm_clk_bulk_get);

static int devm_clk_match(struct device *dev, void *res, void *data)
{
	struct devm_clk_state *state = res;
	if (!state || !state->clk) {
		WARN_ON(!state || !state->clk);
		return 0;
	}
	return state->clk == data;
}

void devm_clk_put(struct device *dev, struct clk *clk)
{
	int ret;

	ret = devres_release(dev, devm_clk_release, devm_clk_match, clk);

	WARN_ON(ret);
}
EXPORT_SYMBOL(devm_clk_put);

struct clk *devm_get_clk_from_child(struct device *dev,
				    struct device_node *np, const char *con_id)
{
	struct devm_clk_state *state, *clk;

	state = devres_alloc(devm_clk_release, sizeof(*state), GFP_KERNEL);
	if (!state)
		return ERR_PTR(-ENOMEM);

	clk = of_clk_get_by_name(np, con_id);
	if (!IS_ERR(clk)) {
		state->clk = clk;
		devres_add(dev, state);
	} else {
		devres_free(state);
	}

	return clk;
}
EXPORT_SYMBOL(devm_get_clk_from_child);
