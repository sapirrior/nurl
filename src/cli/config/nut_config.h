#ifndef NUT_CONFIG_H
#define NUT_CONFIG_H

#include "nut.h"

/**
 * Loads the config file (from NUT_CONFIG env var or ~/.config/nurl/config.toml)
 * and merges the defaults and custom headers into the parsed CommonArgs.
 */
void nut_config_load_and_merge(CommonArgs *args);

#endif /* NUT_CONFIG_H */
