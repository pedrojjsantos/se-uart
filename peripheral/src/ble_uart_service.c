
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>


static ssize_t bt_uart_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 								const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

static void bt_uart_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value);

struct bt_uuid_128 uuid_uart_svc = BT_UUID_INIT_128(BT_UUID_128_ENCODE(1, 2, 3, 4, 0xABCDEF000000));
struct bt_uuid_128 uuid_uart_rx  = BT_UUID_INIT_128(BT_UUID_128_ENCODE(1, 2, 3, 4, 0xABCDEF000001));
struct bt_uuid_128 uuid_uart_tx  = BT_UUID_INIT_128(BT_UUID_128_ENCODE(1, 2, 3, 4, 0xABCDEF000002));

/// Service attribute table
static struct bt_gatt_attr attrs[] = {
		BT_GATT_PRIMARY_SERVICE(&uuid_uart_svc),
		BT_GATT_CHARACTERISTIC(&uuid_uart_rx.uuid, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
													 BT_GATT_PERM_WRITE, NULL, bt_uart_rx, NULL),
		BT_GATT_CHARACTERISTIC(&uuid_uart_tx.uuid, BT_GATT_CHRC_NOTIFY,
													 BT_GATT_PERM_NONE, NULL, NULL, NULL),
		BT_GATT_CCC(bt_uart_ccc_changed, BT_GATT_PERM_WRITE),
};

#define UART_TX_INDEX 3

static struct bt_gatt_service bt_uart_svc = BT_GATT_SERVICE(attrs);

/// Register the bt_uart service
int bt_uart_init() {
	return bt_gatt_service_register(&bt_uart_svc);
}

int bt_uart_transmit(struct bt_conn *conn, const uint8_t *buffer, size_t len) {
	if(!buffer || !len || !conn) {
		return -1;
	}

	return bt_gatt_notify(conn, &attrs[UART_TX_INDEX], buffer, len);
}

static ssize_t bt_uart_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 								const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
	printk("Received: \"%s\"\n",(char*) buf);
	bt_uart_transmit(conn, buf, len);

	return len;
}

static void bt_uart_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value) {
	// TODO:
}
