#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <sys/slist.h>
#include <console/console.h>
#include <stdint.h>

#include <bluetooth/conn.h>
#include <bluetooth/att.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

atomic_t flag_discovery_complete = ATOMIC_INIT(false);

struct bt_uuid_128 uuid_uart_svc = BT_UUID_INIT_128(BT_UUID_128_ENCODE(1, 2, 3, 4, 0xABCDEF000000));
struct bt_uuid_128 uuid_uart_rx  = BT_UUID_INIT_128(BT_UUID_128_ENCODE(1, 2, 3, 4, 0xABCDEF000001));
struct bt_uuid_128 uuid_uart_tx  = BT_UUID_INIT_128(BT_UUID_128_ENCODE(1, 2, 3, 4, 0xABCDEF000002));

static uint16_t rx_handle = 0;
static uint16_t notify_handle = 0;

struct bt_conn *conn_connected;

void start_scanning(void);

uint8_t discovery_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
										 struct bt_gatt_discover_params *params)
{
	if (attr == NULL) {
		if (rx_handle == 0) {
			printk("Did not discover rx_chrc");
		}

		(void)memset(params, 0, sizeof(*params));

		atomic_set(&flag_discovery_complete, true);

		return BT_GATT_ITER_STOP;
	}

	int err;
	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY &&
			bt_uuid_cmp(params->uuid, &uuid_uart_svc.uuid) == 0) 
	{
		printk("Found service\n");

		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, params);
		
		if (err != 0)
			printk("Discover failed (err %d)\n", err);
		
		return BT_GATT_ITER_STOP;
	}

	else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, &uuid_uart_rx.uuid) == 0) {
			printk("Found rx_chrc\n");
			rx_handle = chrc->value_handle;
		}

		else if (bt_uuid_cmp(chrc->uuid, &uuid_uart_tx.uuid) == 0) {
			printk("Found notify_chrc\n");
			notify_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

void discover_handles() {
	printk("Starting Discovery\n");
	
	struct bt_gatt_discover_params discover_params;

	discover_params.uuid = &uuid_uart_svc.uuid;
	discover_params.func = discovery_cb;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xFFFF;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	while (!conn_connected) {
		k_sleep(K_MSEC(200));
	}

	int err = bt_gatt_discover(conn_connected, &discover_params);
	if (err) {
		printk("Discover failed(err %d)\n", err);
	}

	while (!atomic_get(&flag_discovery_complete)) {
		k_sleep(K_MSEC(5));
	}

	printk("Discover complete\n");
}

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
									struct net_buf_simple *ad)
{
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
			type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	if (bt_le_scan_stop()) {
		return;
	}

	int err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &conn_connected);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
		start_scanning();
	}
}

void start_scanning(void) {
	int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);

	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning started\n");
}

void gatt_write(uint16_t handle, const char* data) {
	printk("Sending \"%s\"\n", data);

	int err = bt_gatt_write_without_response(conn_connected, handle, data, strlen(data)+1, false);

	if (err != 0) {
		printk("bt_gatt_write failed (%d)\n", err);
	}
}

void main(void) {
	int err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	start_scanning();
	discover_handles();
	// gatt_subscribe();

	console_getline_init();

	while(true) {
		printk(">");
		char *s = console_getline();
		gatt_write(rx_handle, s);
	}
}
