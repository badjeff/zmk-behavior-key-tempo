/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_key_tempo

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>

#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// NOTE: checked in Kconfig & CMakeLists.txt
// #if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_key_tempo_config {
    uint8_t index;
    uint8_t usage_pages_count;
    uint16_t usage_pages[];
};

struct behavior_key_tempo_data {
    struct zmk_keycode_state_changed last_keycode_pressed;
    struct zmk_keycode_state_changed current_keycode_pressed;
    uint32_t rec_start_ms;
    uint32_t tempo_ms;
    uint32_t hold_ms;
    struct k_work_delayable tempo_press_work;
    struct k_work_delayable tempo_release_work;
};

static void tempo_release_work_handler(struct k_work *work) {
    struct k_work_delayable *work_delayable = (struct k_work_delayable *)work;
    struct behavior_key_tempo_data *data = CONTAINER_OF(work_delayable, 
                                                        struct behavior_key_tempo_data,
                                                        tempo_release_work);

    if (data->tempo_ms) {
        if (data->current_keycode_pressed.usage_page == 0) {
            return;
        }

        data->current_keycode_pressed.timestamp = k_uptime_get();
        data->current_keycode_pressed.state = false;
        raise_zmk_keycode_state_changed(data->current_keycode_pressed);
    }
}

static void tempo_press_work_handler(struct k_work *work) {
    struct k_work_delayable *work_delayable = (struct k_work_delayable *)work;
    struct behavior_key_tempo_data *data = CONTAINER_OF(work_delayable, 
                                                        struct behavior_key_tempo_data,
                                                        tempo_press_work);

    if (data->tempo_ms && data->hold_ms) {
        if (data->current_keycode_pressed.usage_page == 0) {
            return;
        }

        memcpy(&data->current_keycode_pressed, &data->last_keycode_pressed,
            sizeof(struct zmk_keycode_state_changed));
        data->current_keycode_pressed.timestamp = k_uptime_get();
        raise_zmk_keycode_state_changed(data->current_keycode_pressed);
 
        k_work_schedule(&data->tempo_release_work, K_MSEC(data->hold_ms));
        k_work_schedule(&data->tempo_press_work, K_MSEC(data->tempo_ms));
    }
}

static void reset_temop_key(struct behavior_key_tempo_data *data) {
    LOG_DBG("stop tempo key");
    data->rec_start_ms = 0;
    data->tempo_ms = 0;
    data->hold_ms = 0;
}

static void active_tempo_key(struct behavior_key_tempo_data *data) {
    if (data->last_keycode_pressed.usage_page == 0) {
        return;
    }
    if (data->tempo_ms && data->hold_ms) {
        LOG_DBG("start tempo key: %d (ms)", data->tempo_ms);
        k_work_schedule(&data->tempo_press_work, K_MSEC(data->tempo_ms));
    }
}

static int on_key_tempo_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_key_tempo_data *data = dev->data;

    if (data->last_keycode_pressed.usage_page == 0) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    if (data->tempo_ms) {
        reset_temop_key(data);
        return ZMK_BEHAVIOR_OPAQUE;
    }
 
    if (!data->rec_start_ms) {
        data->rec_start_ms = k_uptime_get();
        data->tempo_ms = 0;
        LOG_DBG("rec_start_ms: %d", data->rec_start_ms);
    }
    else {
        data->tempo_ms = k_uptime_get() - data->rec_start_ms;
        data->rec_start_ms = 0;
        LOG_DBG("captured tempo_ms: %d %d", data->tempo_ms, data->hold_ms);
        active_tempo_key(data);
    }

    memcpy(&data->current_keycode_pressed, &data->last_keycode_pressed,
           sizeof(struct zmk_keycode_state_changed));
    data->current_keycode_pressed.timestamp = k_uptime_get();
    raise_zmk_keycode_state_changed(data->current_keycode_pressed);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_key_tempo_binding_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_key_tempo_data *data = dev->data;

    if (data->current_keycode_pressed.usage_page == 0) {
        return ZMK_BEHAVIOR_OPAQUE;
    }

    if (data->rec_start_ms) {
        data->hold_ms = k_uptime_get() - data->rec_start_ms;
    }

    data->current_keycode_pressed.timestamp = k_uptime_get();
    data->current_keycode_pressed.state = false;
    raise_zmk_keycode_state_changed(data->current_keycode_pressed);

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_key_tempo_driver_api = {
    .binding_pressed = on_key_tempo_binding_pressed,
    .binding_released = on_key_tempo_binding_released,
};

static const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

static int key_tempo_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); i++) {
        const struct device *dev = devs[i];
        if (dev == NULL) {
            continue;
        }

        struct behavior_key_tempo_data *data = dev->data;
        const struct behavior_key_tempo_config *config = dev->config;

        for (int u = 0; u < config->usage_pages_count; u++) {
            if (config->usage_pages[u] == ev->usage_page) {
                memcpy(&data->last_keycode_pressed, ev, sizeof(struct zmk_keycode_state_changed));
                data->last_keycode_pressed.implicit_modifiers |= zmk_hid_get_explicit_mods();

                // intercept with any other keycode pressed, then stop tempo
                if (data->current_keycode_pressed.usage_page) {
                    if (data->tempo_ms || data->rec_start_ms) {
                        if (data->current_keycode_pressed.keycode != data->last_keycode_pressed.keycode) {
                            LOG_DBG("tempo key intercepted with different keycode");
                            reset_temop_key(data);                            
                        }
                    }
                }

                break;
            }
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(behavior_key_tempo, key_tempo_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_key_tempo, zmk_keycode_state_changed);

static int behavior_key_tempo_init(const struct device *dev) {
    const struct behavior_key_tempo_config *config = dev->config;
    devs[config->index] = dev;

    struct behavior_key_tempo_data *data = dev->data;
    k_work_init_delayable(&data->tempo_press_work, tempo_press_work_handler);
    k_work_init_delayable(&data->tempo_release_work, tempo_release_work_handler);

    return 0;
}

#define KR_INST(n)                                                                                 \
    static struct behavior_key_tempo_data behavior_key_tempo_data_##n = {};                      \
    static struct behavior_key_tempo_config behavior_key_tempo_config_##n = {                    \
        .index = n,                                                                                \
        .usage_pages = DT_INST_PROP(n, usage_pages),                                               \
        .usage_pages_count = DT_INST_PROP_LEN(n, usage_pages),                                     \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_key_tempo_init, NULL, &behavior_key_tempo_data_##n,      \
                            &behavior_key_tempo_config_##n, POST_KERNEL,                          \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_key_tempo_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KR_INST)

// #endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
