# Home Assistant Dashboard for Project Aura

This folder contains a ready-to-use YAML configuration to visualize your Aura data.
It uses only standard Home Assistant cards. No HACS or external dependencies required.
The example view includes dedicated `CO2` and `AQI` gauge cards at the top, plus a
standard Home Assistant ventilation section with explicit `Auto` / `Manual` / `Stop`
mode switches, a manual speed slider in percent, a timer selector, and a timer
remaining indicator for the optional DAC-based exhaust output.

![Dashboard Preview](../assets/preview.png)

## How to Add
You can add this as a new view (tab) to your existing dashboard.

1. Open your Home Assistant Dashboard.
2. Click the Pencil icon (Edit Dashboard) in the top right.
3. Click the 3 dots menu (top right) -> Raw configuration editor.
4. Scroll to the place where you want to insert the code (or replace existing code if starting fresh).
5. Paste the contents of `dashboard.yaml`.
6. Click Save.

## Entity Configuration
Entity IDs are derived from your `MQTT base topic`.

If your base topic is the default `project_aura`, the example YAML should match directly:
- `sensor.project_aura_temperature`
- `switch.project_aura_fan_auto`
- `select.project_aura_fan_timer`

If you use a custom base topic, replace the `project_aura_` prefix in the YAML with the
slugged base topic. For example:
- base topic `project_aura_kitchen` -> `sensor.project_aura_kitchen_temperature`
- base topic `project_aura/bedroom` -> `sensor.project_aura_bedroom_temperature`

If you see "Entity not found" warnings:
1. Go to Settings -> Devices & Services -> Entities.
2. Search for your MQTT base topic or `Aura`.
3. Open the dashboard code and use Find & Replace to swap the prefix.

## Multiple Aura Devices
Two Aura devices must use different `MQTT base topic` values.

This is important for two reasons:
1. State and command topics are built from the base topic.
2. Home Assistant entity IDs are also derived from the base topic.

Recommended examples:
- `project_aura_kitchen`
- `project_aura_bedroom`
- `project_aura_office`
