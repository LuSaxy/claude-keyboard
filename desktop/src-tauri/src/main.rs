// Hide the extra console window on Windows release builds.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod ble;
mod clawd;

fn main() {
    tauri::Builder::default()
        .manage(ble::AppState::default())
        .invoke_handler(tauri::generate_handler![
            ble::scan_devices,
            ble::connect_device,
            ble::disconnect,
            ble::read_state,
            ble::set_led,
            ble::set_keymap,
            ble::set_device_name,
            ble::reset_keymap,
            ble::reset_name,
        ])
        .run(tauri::generate_context!())
        .expect("error while running Clawd Control");
}
