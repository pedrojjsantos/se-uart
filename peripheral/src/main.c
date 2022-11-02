#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <kernel.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

extern int bt_uart_init();

struct bt_conn *default_conn;

static const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_128_ENCODE(1, 2, 3, 4, 0xABCDEF000000)),
};

static void connected(struct bt_conn *conn, uint8_t err) {
	if (err) {
		printk("Connection failed (err %u)\n", err);
	}
	else {
		default_conn = bt_conn_ref(conn);
		printk("Connected :)\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
	printk("Disconnected (reason %u)\n", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
};

static void bt_ready(int err) {
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	err = bt_uart_init();

	if (err) {
		printk("Service bt_uart failed to start (err %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

}

void main(void) {
	printk("starting\n");

	if (bt_enable(bt_ready)) {
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
}
