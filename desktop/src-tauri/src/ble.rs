//! Native BLE backend for the Clawd desktop app.
//!
//! All Bluetooth work happens here in Rust via `btleplug`; the web view only
//! calls these commands through `invoke`. Encrypted characteristics require the
//! device to be bonded/paired at the OS level first (the same bond nRF Connect
//! creates).

use std::collections::HashMap;
use std::time::Duration;

use btleplug::api::{Central, Characteristic, Manager as _, Peripheral as _, ScanFilter, WriteType};
use btleplug::platform::{Adapter, Manager, Peripheral};
use serde::Serialize;
use tokio::sync::Mutex;
use tokio::time::sleep;

use crate::clawd;

/// Shared, Tauri-managed state. `tokio::sync::Mutex` so we can hold it across
/// `.await` points safely.
#[derive(Default)]
pub struct AppState {
    inner: Mutex<Ble>,
}

#[derive(Default)]
struct Ble {
    adapter: Option<Adapter>,
    discovered: HashMap<String, Peripheral>,
    connected: Option<Peripheral>,
}

#[derive(Serialize, Clone)]
pub struct DeviceInfo {
    pub id: String,
    pub name: String,
    pub looks_like_clawd: bool,
}

#[derive(Serialize, Clone)]
pub struct ClawdState {
    pub name: String,
    pub led: u8,
    pub keymap: Vec<u8>,
}

fn e2s<E: std::fmt::Display>(e: E) -> String {
    e.to_string()
}

fn peripheral_id(p: &Peripheral) -> String {
    format!("{:?}", p.id())
}

async fn ensure_adapter(state: &mut Ble) -> Result<Adapter, String> {
    if let Some(adapter) = &state.adapter {
        return Ok(adapter.clone());
    }
    let manager = Manager::new().await.map_err(e2s)?;
    let adapters = manager.adapters().await.map_err(e2s)?;
    let adapter = adapters
        .into_iter()
        .next()
        .ok_or_else(|| "No Bluetooth adapter found".to_string())?;
    state.adapter = Some(adapter.clone());
    Ok(adapter)
}

async fn connected_peripheral(state: &AppState) -> Result<Peripheral, String> {
    let guard = state.inner.lock().await;
    guard
        .connected
        .clone()
        .ok_or_else(|| "Not connected to a device".to_string())
}

fn find_char(p: &Peripheral, uuid: uuid::Uuid) -> Result<Characteristic, String> {
    p.characteristics()
        .into_iter()
        .find(|c| c.uuid == uuid)
        .ok_or_else(|| format!("Characteristic {uuid} not found (is this a Clawd device?)"))
}

async fn read_clawd_state(p: &Peripheral) -> Result<ClawdState, String> {
    let led_char = find_char(p, clawd::LED_CHAR)?;
    let name_char = find_char(p, clawd::NAME_CHAR)?;
    let keymap_char = find_char(p, clawd::KEYMAP_CHAR)?;

    let led = p
        .read(&led_char)
        .await
        .map_err(e2s)?
        .first()
        .copied()
        .unwrap_or(0);

    let name_bytes = p.read(&name_char).await.map_err(e2s)?;
    let name = String::from_utf8_lossy(&name_bytes)
        .trim_end_matches('\0')
        .to_string();

    let keymap = p.read(&keymap_char).await.map_err(e2s)?;

    Ok(ClawdState { name, led, keymap })
}

/// Scan for nearby BLE devices for a few seconds and remember them so a later
/// `connect_device` call can look them up by id.
#[tauri::command]
pub async fn scan_devices(
    state: tauri::State<'_, AppState>,
    seconds: Option<u64>,
) -> Result<Vec<DeviceInfo>, String> {
    let adapter = {
        let mut guard = state.inner.lock().await;
        ensure_adapter(&mut guard).await?
    };

    adapter
        .start_scan(ScanFilter::default())
        .await
        .map_err(e2s)?;
    sleep(Duration::from_secs(seconds.unwrap_or(4))).await;
    let _ = adapter.stop_scan().await;

    let peripherals = adapter.peripherals().await.map_err(e2s)?;

    let mut guard = state.inner.lock().await;
    guard.discovered.clear();

    let mut out = Vec::new();
    for p in peripherals {
        let name = p
            .properties()
            .await
            .map_err(e2s)?
            .and_then(|props| props.local_name)
            .unwrap_or_default();
        let id = peripheral_id(&p);
        let looks_like_clawd = name.to_lowercase().contains("clawd");
        guard.discovered.insert(id.clone(), p);
        out.push(DeviceInfo {
            id,
            name,
            looks_like_clawd,
        });
    }

    // Clawd-looking, then named, then the rest.
    out.sort_by(|a, b| {
        b.looks_like_clawd
            .cmp(&a.looks_like_clawd)
            .then_with(|| b.name.is_empty().cmp(&a.name.is_empty()))
            .then_with(|| a.name.to_lowercase().cmp(&b.name.to_lowercase()))
    });

    Ok(out)
}

/// Connect to a previously scanned device, discover services, and return the
/// current LED / name / keymap state.
#[tauri::command]
pub async fn connect_device(
    state: tauri::State<'_, AppState>,
    id: String,
) -> Result<ClawdState, String> {
    let peripheral = {
        let guard = state.inner.lock().await;
        guard
            .discovered
            .get(&id)
            .cloned()
            .ok_or_else(|| "Device not in scan results; scan again".to_string())?
    };

    if !peripheral.is_connected().await.map_err(e2s)? {
        peripheral.connect().await.map_err(e2s)?;
    }
    peripheral.discover_services().await.map_err(e2s)?;

    // Make sure this is actually a Clawd device before reading.
    let services = peripheral.services();
    let has = |uuid| services.iter().any(|s| s.uuid == uuid);
    if !has(clawd::LED_SERVICE) || !has(clawd::CONFIG_SERVICE) {
        return Err("This device is not a Clawd (missing LED/config services)".to_string());
    }

    let state_snapshot = read_clawd_state(&peripheral).await?;

    {
        let mut guard = state.inner.lock().await;
        guard.connected = Some(peripheral);
    }

    Ok(state_snapshot)
}

#[tauri::command]
pub async fn disconnect(state: tauri::State<'_, AppState>) -> Result<(), String> {
    let peripheral = {
        let mut guard = state.inner.lock().await;
        guard.connected.take()
    };
    if let Some(p) = peripheral {
        let _ = p.disconnect().await;
    }
    Ok(())
}

#[tauri::command]
pub async fn read_state(state: tauri::State<'_, AppState>) -> Result<ClawdState, String> {
    let peripheral = connected_peripheral(&state).await?;
    read_clawd_state(&peripheral).await
}

#[tauri::command]
pub async fn set_led(state: tauri::State<'_, AppState>, level: u8) -> Result<(), String> {
    let peripheral = connected_peripheral(&state).await?;
    let led_char = find_char(&peripheral, clawd::LED_CHAR)?;
    peripheral
        .write(&led_char, &[level], WriteType::WithResponse)
        .await
        .map_err(e2s)
}

#[tauri::command]
pub async fn set_keymap(state: tauri::State<'_, AppState>, bytes: Vec<u8>) -> Result<(), String> {
    if bytes.len() != clawd::KEYMAP_LEN {
        return Err(format!("Keymap must be {} bytes", clawd::KEYMAP_LEN));
    }
    let peripheral = connected_peripheral(&state).await?;
    let keymap_char = find_char(&peripheral, clawd::KEYMAP_CHAR)?;
    peripheral
        .write(&keymap_char, &bytes, WriteType::WithResponse)
        .await
        .map_err(e2s)
}

#[tauri::command]
pub async fn set_device_name(state: tauri::State<'_, AppState>, name: String) -> Result<(), String> {
    let bytes = name.as_bytes();
    if bytes.is_empty() || bytes.len() > clawd::NAME_MAX_LEN {
        return Err(format!("Name must be 1-{} bytes", clawd::NAME_MAX_LEN));
    }
    if !bytes.iter().all(|b| (32..=126).contains(b)) {
        return Err("Name must be printable ASCII".to_string());
    }
    let peripheral = connected_peripheral(&state).await?;
    let name_char = find_char(&peripheral, clawd::NAME_CHAR)?;
    peripheral
        .write(&name_char, bytes, WriteType::WithResponse)
        .await
        .map_err(e2s)
}

async fn write_command(state: &AppState, command: u8) -> Result<ClawdState, String> {
    let peripheral = connected_peripheral(state).await?;
    let cmd_char = find_char(&peripheral, clawd::COMMAND_CHAR)?;
    peripheral
        .write(&cmd_char, &[command], WriteType::WithResponse)
        .await
        .map_err(e2s)?;
    // Give the firmware a moment to apply + persist, then re-read.
    sleep(Duration::from_millis(150)).await;
    read_clawd_state(&peripheral).await
}

#[tauri::command]
pub async fn reset_keymap(state: tauri::State<'_, AppState>) -> Result<ClawdState, String> {
    write_command(&state, clawd::CMD_RESET_KEYMAP).await
}

#[tauri::command]
pub async fn reset_name(state: tauri::State<'_, AppState>) -> Result<ClawdState, String> {
    write_command(&state, clawd::CMD_RESET_NAME).await
}
