# 🤖 Hexadrone

[▶️ Video Demonstration](https://www.youtube.com/watch?v=5gf2d8lJZK4)

A versatile, ROS 2 and ESP32 compatible software stack for a 18-DOF hexapod robot (drone). This repository contains the unified C++ brain, kinematics engine, Webots simulation environment, and the physical PlatformIO hardware layer.

📖 **For full hardware specifications, control mappings, architecture details, and system logic, please read the [Documentation](DOCUMENTATION.md).**

---

### 🛠️ Build Instructions
This ESP32 project relies on a shared core logic library. You must clone the entire ROS 2 workspace to maintain the correct folder structure:

1. Clone the complete workspace: `git clone yehorderehus/hexadrone`
2. Ensure `hexadrone_core` and `hexadrone_esp32` exist in the same parent directory.
3. Open the `hexadrone_esp32` folder in PlatformIO.
4. Click **Build**. PlatformIO will automatically resolve the symlink and download all external I2C/Radio libraries.

---

## 📡 Wireless Connection & OTA Updates

The Hexadrone features a state-aware WiFi manager designed to prioritize radio control safety. To prevent 2.4GHz interference with the ELRS receiver, **WiFi is strictly disabled while the drone is ARMED.** To connect to the drone wirelessly for diagnostics or code updates, you must be in the **DISARMED** or **OE-Kill** state.

### 1. Connecting to the Drone
1. **Configure your Network:** Ensure your local WiFi credentials (SSID and Password) are correctly set in `include/config.h`.
2. **Disarm the Drone:** Ensure your radio's ARM switch (Switch SA) is in the low/disarmed position. 
3. **Power On:** Boot the drone. The ESP32 will automatically connect to your designated WiFi network.
4. **Access the Network:** Make sure your PC or mobile device is connected to the same WiFi network. The drone will broadcast its presence via mDNS.

### 2. The Gunslinger Web Terminal
You do not need a USB cable to retrieve crash or walking data. While the drone is disarmed and connected to WiFi:
* Open any web browser on your connected device.
* Navigate to: `http://hexadrone.local/` (or use the direct IP address).
* You will be greeted by the **Maintenance Protocol** terminal. From here, you can download the `system.log`, retrieve the `power.csv` telemetry data, or wipe all internal logs from the LittleFS partition.

### 3. Over-The-Air (OTA) Firmware Updates
You can completely rewrite the ESP32's firmware wirelessly using PlatformIO's built-in ArduinoOTA support.
- To ensure maximum network stability and prevent hardware timeouts during Over-The-Air (OTA) updates, it is highly recommended to use the raw PlatformIO Command Line Interface (CLI) rather than the built-in VS Code "Upload" button.

```ini
~/.platformio/penv/bin/pio run -t upload
```

For wireless uploads, uncomment these lines in `platformio.ini` and set `upload_port` to the drone's IP address. If they remain commented out, uploads will use the wired connection:

```ini
upload_port = 10.46.34.101
upload_protocol = espota
```

---

## 💻 Simulation Reference

### Prerequisites

The simulation requires ROS 2 Jazzy and Webots R2025a. Configure both on an Ubuntu 25.10 (the tested version) before running the simulation.

Execute these commands from your host terminal to manage the ROS 2 Jazzy simulation environment using **Makefile**.

| Command                 | Action                                                                 |
|-------------------------|------------------------------------------------------------------------|
| **`make up`**           | **Start** the Podman container in the background.                      |
| **`make build`**        | **Build** the core, controller, and Webots launch/plugin packages.     |
| **`make run`**          | **Launch** Webots and ROS 2 nodes. Press **Ctrl+C** to exit.           |
| **`make run-keyboard`** | **Launch** Webots without `joy_node`, for keyboard-only control.       |
| **`make stop`**         | **Kill** all container processes and stop the environment.             |
| **`make shell`**        | **Enter** the interactive **fish** shell inside the container.         |

---

### Debugging & Diagnostics (Shell Tab)
Use these commands inside `make shell` while the simulation is running to inspect the drone's state.

| Command                                                 | Purpose                                                             |
|---------------------------------------------------------|---------------------------------------------------------------------|
| `ros2 topic echo /joy`                                  | Monitors input from the enabled joystick and/or keyboard plugin.    |
| `ros2 topic echo /forward_position_controller/commands` | Monitors the 18 servo angles being sent to Webots.                  |
| `ros2 topic hz /joy`                                    | Shows the frequency of messages from the active input publisher.   |
| `ros2 node info /teleop_node`                           | Shows which topics the Brain is currently reading and writing.      |
| `ros2 param list`                                       | Lists all active configuration parameters for the servos.           |
| `ros2 doctor`                                           | Runs a global diagnostic on the ROS 2 container environment.        |

### ROS 2 /joy Topic Index Map in bridge_ops.hpp

| Index Type  | Index      | Physical Control | Range / Logic (In C++)                        |
|-------------|------------|------------------|-----------------------------------------------|
| **Axes**    | `[0]`      | Right Stick (X)  | -1.0 (Right) to 1.0 (Left) [Inverted]         |
| **Axes**    | `[1]`      | Right Stick (Y)  | -1.0 (Down) to 1.0 (Up) [Inverted]            |
| **Axes**    | `[2]`      | Left Stick (Y)   | -1.0 (Down) to 1.0 (Up) [Inverted]            |
| **Axes**    | `[3]`      | Left Stick (X)   | -1.0 (Right) to 1.0 (Left) [Inverted]         |
| **Axes**    | `[4]`      | Switch SB        | -1.0 (Up) / 0.0 (Mid) / 1.0 (Down) [Inverted] |
| **Axes**    | `[5]`      | Knob S1          | -1.0 (Right) to 1.0 (Left) [Inverted]         |
| **Buttons** | `[0]`      | Switch SA        | 0 (Low) / 1 (High)                            |
| **Buttons** | `[1]`      | Button SE        | 0 (Off) / 1 (On)                              |
| **Buttons** | `[2]`      | Switch SD        | 0 (Low) / 1 (High)                            |
| **Buttons** | `[3, 4]`   | Right Trim (X)   | 3: -1 (Left) / 4: 1 (Right)                   |
| **Buttons** | `[5, 6]`   | Right Trim (Y)   | 5: -1 (Down) / 6: 1 (Up)                      |
| **Buttons** | `[7, 8]`   | Left Trim (Y)    | 7: -1 (Down) / 8: 1 (Up)                      |
| **Buttons** | `[9, 10]`  | Left Trim (X)    | 9: Prev Leg / 10: Next Leg                    |
| **Buttons** | `[11, 12]` | Switch SC        | 11: Forward (-1) / 12: Backward (1)           |

### Webots Simulation Controls

| Keys                            | Function                            |
|---------------------------------|-------------------------------------|
| `W` / `S`                       | Walk forward / backward             |
| `A` / `D`                       | Yaw left / right                    |
| `I` / `K`                       | Pitch forward / backward            |
| `J` / `L`                       | Roll left / right                   |
| `Space`                         | Toggle arm / disarm                 |
| `Z` / `X` / `C`                 | Crouch / standard / high posture    |
| `Q` / `E`                       | Select previous / next leg          |
| `1` / `2`, `3` / `4`, `5` / `6` | Adjust coxa, femur, and tibia trims |
| `B`                             | Hold the OE-kill button             |
