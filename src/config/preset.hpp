#pragma once

#define MODKEY WLR_MODIFIER_ALT

static const char *tags[] = {
	"1", "2", "3", "4", "5", "6", "7", "8", "9",
};

static const struct xkb_rule_names xkb_fallback_rules = {
	.rules = nullptr,
	.model = nullptr,
	.layout = "us",
	.variant = nullptr,
	.options = nullptr,
};
