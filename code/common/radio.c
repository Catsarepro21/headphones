#include "radio.h"
#include "audio.h"

#include <errno.h>
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>

#define SERVICE_ID BT_UUID_128_ENCODE(0xccb18a1c, 0x3abf, 0x4ef4, 0x8f9c, 0x36751d83ba9c)
#define PLAY_ID BT_UUID_128_ENCODE(0xccb18a1d, 0x3abf, 0x4ef4, 0x8f9c, 0x36751d83ba9c)
#define MIC_ID BT_UUID_128_ENCODE(0xccb18a1e, 0x3abf, 0x4ef4, 0x8f9c, 0x36751d83ba9c)
#define FAST_INTERVAL BT_LE_CONN_PARAM(6, 6, 0, 200)

enum { SEND_SLOTS = 4 };

static struct k_spinlock conn_lock;
static struct bt_conn *current;
static radio_receive_fn receive_packet;
static atomic_t ready;
static atomic_t running;
static atomic_t link_generation;
K_SEM_DEFINE(send_slots, SEND_SLOTS, SEND_SLOTS);
static uint32_t last_poll;
static const struct bt_uuid *const service_uuid = BT_UUID_DECLARE_128(SERVICE_ID);
static const struct bt_uuid *const play_uuid = BT_UUID_DECLARE_128(PLAY_ID);
static const struct bt_uuid *const mic_uuid = BT_UUID_DECLARE_128(MIC_ID);

static struct bt_conn *connection_ref(void)
{
    k_spinlock_key_t key = k_spin_lock(&conn_lock);
    struct bt_conn *const connection = current == NULL ? NULL : bt_conn_ref(current);
    k_spin_unlock(&conn_lock, key);
    return connection;
}

static void disconnected(struct bt_conn *connection, uint8_t reason)
{
    ARG_UNUSED(reason);
    k_spinlock_key_t key = k_spin_lock(&conn_lock);
    if (current == connection) {
        current = NULL;
        bt_conn_unref(connection);
    }
    atomic_clear(&ready);
    atomic_clear(&running);
    k_spin_unlock(&conn_lock, key);
}

static void sent(struct bt_conn *connection, void *data)
{
    ARG_UNUSED(connection);
    if ((uintptr_t)data == (uintptr_t)atomic_get(&link_generation)) {
        k_sem_give(&send_slots);
    }
}

#ifdef HEADPHONES_DONGLE

static struct bt_gatt_discover_params discovery;
static struct bt_gatt_discover_params ccc_discovery;
static struct bt_gatt_subscribe_params subscribe;
static struct bt_gatt_exchange_params exchange;
static atomic_t play_handle;
static uint16_t service_end;

static uint8_t on_notify(struct bt_conn *connection, struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length)
{
    ARG_UNUSED(connection);
    ARG_UNUSED(params);
    if (data == NULL) {
        atomic_clear(&ready);
        return BT_GATT_ITER_STOP;
    }
    receive_packet(data, length);
    return BT_GATT_ITER_CONTINUE;
}

static void subscribed(struct bt_conn *connection, uint8_t error,
                       struct bt_gatt_subscribe_params *params)
{
    ARG_UNUSED(params);
    if (error != 0U) {
        bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return;
    }
    atomic_set(&ready, 1);
}

static uint8_t discovered(struct bt_conn *connection, const struct bt_gatt_attr *attribute,
                          struct bt_gatt_discover_params *params)
{
    if (attribute == NULL) {
        bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return BT_GATT_ITER_STOP;
    }
    if (params->type == BT_GATT_DISCOVER_PRIMARY) {
        const struct bt_gatt_service_val *service = attribute->user_data;
        service_end = service->end_handle;
        params->uuid = play_uuid;
        params->start_handle = attribute->handle + 1U;
        params->end_handle = service_end;
        params->type = BT_GATT_DISCOVER_CHARACTERISTIC;
    } else if (bt_uuid_cmp(params->uuid, play_uuid) == 0) {
        atomic_set(&play_handle, bt_gatt_attr_value_handle(attribute));
        params->uuid = mic_uuid;
        params->start_handle = attribute->handle + 1U;
    } else {
        subscribe = (struct bt_gatt_subscribe_params){
            .notify = on_notify,
            .subscribe = subscribed,
            .value_handle = bt_gatt_attr_value_handle(attribute),
            .ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
            .value = BT_GATT_CCC_NOTIFY,
            .end_handle = service_end,
            .disc_params = &ccc_discovery,
            .min_security = BT_SECURITY_L2,
        };
        atomic_set_bit(subscribe.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
        if (bt_gatt_subscribe(connection, &subscribe) < 0) {
            bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        }
        return BT_GATT_ITER_STOP;
    }
    if (bt_gatt_discover(connection, params) < 0) {
        bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
    return BT_GATT_ITER_STOP;
}

static void mtu_exchanged(struct bt_conn *connection, uint8_t error,
                          struct bt_gatt_exchange_params *params)
{
    ARG_UNUSED(params);
    if (error != 0U || bt_gatt_get_mtu(connection) < AUDIO_PACKET_MIN_MTU) {
        bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return;
    }
    discovery = (struct bt_gatt_discover_params){
        .uuid = service_uuid,
        .func = discovered,
        .start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
        .type = BT_GATT_DISCOVER_PRIMARY,
    };
    if (bt_gatt_discover(connection, &discovery) < 0) {
        bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

static void security_changed(struct bt_conn *connection, bt_security_t level,
                             enum bt_security_err error)
{
    if (error != BT_SECURITY_ERR_SUCCESS || level < BT_SECURITY_L2) {
        bt_conn_disconnect(connection, BT_HCI_ERR_AUTH_FAIL);
        return;
    }
    exchange.func = mtu_exchanged;
    const int status = bt_gatt_exchange_mtu(connection, &exchange);
    if (status == -EALREADY) {
        mtu_exchanged(connection, 0, &exchange);
    } else if (status < 0) {
        bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

static bool find_service(struct bt_data *data, void *found)
{
    if (data->type != BT_DATA_UUID128_ALL && data->type != BT_DATA_UUID128_SOME) {
        return true;
    }
    const struct bt_uuid_128 *uuid = BT_UUID_128(service_uuid);
    for (size_t position = 0; position + sizeof(uuid->val) <= data->data_len;
         position += sizeof(uuid->val)) {
        if (memcmp(data->data + position, uuid->val, sizeof(uuid->val)) == 0) {
            *(bool *)found = true;
            return false;
        }
    }
    return true;
}

struct bond_match {
    const bt_addr_le_t *address;
    bool any;
    bool match;
};

static void match_bond(const struct bt_bond_info *bond, void *context)
{
    struct bond_match *match = context;
    match->any = true;
    match->match |= bt_addr_le_eq(&bond->addr, match->address);
}

static void scanned(const bt_addr_le_t *address, int8_t rssi, uint8_t type,
                    struct net_buf_simple *advertisement)
{
    ARG_UNUSED(rssi);
    if (type != BT_GAP_ADV_TYPE_ADV_IND) {
        return;
    }
    struct bond_match match = {.address = address};
    bt_foreach_bond(BT_ID_DEFAULT, match_bond, &match);
    if (match.any && !match.match) {
        return;
    }
    bool found = false;
    bt_data_parse(advertisement, find_service, &found);
    if (!found || bt_le_scan_stop() != 0) {
        return;
    }
    struct bt_conn *connection;
    if (bt_conn_le_create(address, BT_CONN_LE_CREATE_CONN, FAST_INTERVAL, &connection) == 0) {
        bt_conn_unref(connection);
    } else {
        atomic_clear(&running);
    }
}

#else

static atomic_t available;
static atomic_t pair_until;
static atomic_t bond_exists;
static int filter_status;

static const struct bt_data advertising[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, SERVICE_ID),
};

static ssize_t on_write(struct bt_conn *connection, const struct bt_gatt_attr *attribute,
                        const void *data, uint16_t length, uint16_t offset, uint8_t flags)
{
    ARG_UNUSED(connection);
    ARG_UNUSED(attribute);
    ARG_UNUSED(flags);
    if (!atomic_get(&available) || offset != 0U || length > AUDIO_PACKET_MAX) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }
    receive_packet(data, length);
    return length;
}

static void ccc_changed(const struct bt_gatt_attr *attribute, uint16_t value)
{
    ARG_UNUSED(attribute);
    atomic_set(&ready, value == BT_GATT_CCC_NOTIFY);
}

BT_GATT_SERVICE_DEFINE(audio_service,
    BT_GATT_PRIMARY_SERVICE(service_uuid),
    BT_GATT_CHARACTERISTIC(play_uuid, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE_ENCRYPT, NULL, on_write, NULL),
    BT_GATT_CHARACTERISTIC(mic_uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT)
);

static void add_bond(const struct bt_bond_info *bond, void *context)
{
    ARG_UNUSED(context);
    atomic_set(&bond_exists, 1);
    filter_status = bt_le_filter_accept_list_add(&bond->addr);
}

static bool pairing_open(void)
{
    const uint32_t until = (uint32_t)atomic_get(&pair_until);
    return until != 0U && (int32_t)(until - k_uptime_get_32()) > 0;
}

static void pairing_complete(struct bt_conn *connection, bool bonded)
{
    ARG_UNUSED(connection);
    if (bonded) {
        atomic_clear(&pair_until);
    }
}

static struct bt_conn_auth_info_cb auth_info = {
    .pairing_complete = pairing_complete,
};

void radio_set_available(bool enabled)
{
    atomic_set(&available, enabled);
    if (!enabled) {
        atomic_clear(&pair_until);
        atomic_clear(&ready);
        struct bt_conn *connection = connection_ref();
        if (connection != NULL) {
            bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            bt_conn_unref(connection);
        } else if (atomic_get(&running) && bt_le_adv_stop() == 0) {
            atomic_clear(&running);
        }
    }
}

void radio_allow_pairing(void)
{
    if (atomic_get(&available)) {
        atomic_set(&pair_until, (atomic_val_t)(k_uptime_get_32() + 30000U));
    }
}

#endif

static void connected(struct bt_conn *connection, uint8_t error)
{
    if (error != 0U) {
        atomic_clear(&running);
        return;
    }
    atomic_inc(&link_generation);
    k_sem_reset(&send_slots);
    for (unsigned int i = 0; i < SEND_SLOTS; ++i) {
        k_sem_give(&send_slots);
    }
    k_spinlock_key_t key = k_spin_lock(&conn_lock);
    current = bt_conn_ref(connection);
    k_spin_unlock(&conn_lock, key);
#ifdef HEADPHONES_DONGLE
    if (bt_conn_set_security(connection, BT_SECURITY_L2) < 0) {
        bt_conn_disconnect(connection, BT_HCI_ERR_AUTH_FAIL);
    }
#else
    if (!atomic_get(&available) || (!atomic_get(&bond_exists) && !pairing_open())) {
        bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
#endif
}

BT_CONN_CB_DEFINE(radio_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
#ifdef HEADPHONES_DONGLE
    .security_changed = security_changed,
#endif
};

int radio_start(radio_receive_fn receive)
{
    receive_packet = receive;
    int status = bt_enable(NULL);
    if (status < 0) {
        return status;
    }
    status = settings_load();
#ifndef HEADPHONES_DONGLE
    if (status == 0) {
        status = bt_conn_auth_info_cb_register(&auth_info);
    }
#endif
    return status;
}

void radio_poll(void)
{
    const uint32_t now = k_uptime_get_32();
    if (now - last_poll < 200U) {
        return;
    }
    last_poll = now;
#ifdef HEADPHONES_DONGLE
    if (atomic_cas(&running, 0, 1) && bt_le_scan_start(BT_LE_SCAN_PASSIVE, scanned) != 0) {
        atomic_clear(&running);
    }
#else
    if (!atomic_get(&available)) {
        return;
    }
    if (atomic_get(&running)) {
        if (!atomic_get(&bond_exists) && !pairing_open()) {
            struct bt_conn *connection = connection_ref();
            if (connection == NULL && bt_le_adv_stop() == 0) {
                atomic_clear(&running);
            }
            if (connection != NULL) {
                bt_conn_unref(connection);
            }
        }
        return;
    }
    if (bt_le_filter_accept_list_clear() != 0) {
        return;
    }
    atomic_clear(&bond_exists);
    filter_status = 0;
    bt_foreach_bond(BT_ID_DEFAULT, add_bond, NULL);
    const bool bonded = atomic_get(&bond_exists) != 0;
    if (filter_status != 0 || (!bonded && !pairing_open())) {
        return;
    }
    const uint32_t flags = BT_LE_ADV_OPT_CONN | (bonded ? BT_LE_ADV_OPT_FILTER_CONN : 0);
    if (atomic_cas(&running, 0, 1) &&
        bt_le_adv_start(BT_LE_ADV_PARAM(flags, 0x30, 0x60, NULL),
                        advertising, ARRAY_SIZE(advertising), NULL, 0) != 0) {
        atomic_clear(&running);
    }
#endif
}

bool radio_active(void)
{
    return atomic_get(&ready) != 0;
}

int radio_send(const uint8_t *packet, size_t length)
{
    if (!radio_active() || length > AUDIO_PACKET_MAX) {
        return -ENOTCONN;
    }
    struct bt_conn *connection = connection_ref();
    if (connection == NULL) {
        return -ENOTCONN;
    }
    int status = -EACCES;
    if (bt_conn_get_security(connection) >= BT_SECURITY_L2 &&
        bt_gatt_get_mtu(connection) >= AUDIO_PACKET_MIN_MTU) {
        void *const generation = (void *)(uintptr_t)atomic_get(&link_generation);
        if (k_sem_take(&send_slots, K_NO_WAIT) < 0) {
            bt_conn_unref(connection);
            return -EAGAIN;
        }
#ifdef HEADPHONES_DONGLE
        status = bt_gatt_write_without_response_cb(
            connection, (uint16_t)atomic_get(&play_handle), packet, length, false, sent, generation);
#else
        struct bt_gatt_notify_params tx = {
            .attr = &audio_service.attrs[4],
            .data = packet,
            .len = length,
            .func = sent,
            .user_data = generation,
        };
        status = bt_gatt_notify_cb(connection, &tx);
#endif
        if (status < 0) {
            sent(connection, generation);
        }
    }
    bt_conn_unref(connection);
    return status;
}
