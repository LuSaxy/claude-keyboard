//! Clawd BLE protocol constants. These mirror the firmware UUIDs in
//! `program/xiao_ble_keyboard/xiao_ble_keyboard.ino`.

use uuid::Uuid;

// Custom "user LED" service: a single byte = LED brightness (0..255).
pub const LED_SERVICE: Uuid = Uuid::from_u128(0x494dceab_1533_4012_afaa_20d5acfc6ecf);
pub const LED_CHAR: Uuid = Uuid::from_u128(0xbc6df096_9c5a_4034_89d2_8440c14d2ab0);

// Custom "config" service: keymap (8 B), device name (<=15 B), command (1 B).
pub const CONFIG_SERVICE: Uuid = Uuid::from_u128(0xe0934030_b408_4dcc_b4ca_56d83d7c5fbc);
pub const KEYMAP_CHAR: Uuid = Uuid::from_u128(0xe0c5d214_8809_4243_afa2_9862d3628c38);
pub const NAME_CHAR: Uuid = Uuid::from_u128(0x8adf91b2_c9a1_4c1a_8b76_2ff5be6b86de);
pub const COMMAND_CHAR: Uuid = Uuid::from_u128(0x37508895_a542_4ee0_9ab4_b109ce4717de);

// Command characteristic values.
pub const CMD_RESET_KEYMAP: u8 = 0x01;
pub const CMD_RESET_NAME: u8 = 0x02;

// Wire format limits.
pub const NAME_MAX_LEN: usize = 15;
pub const KEYMAP_LEN: usize = 8;
