.PHONY: up shell build run run-keyboard stop deploy posture-prone posture-standard posture-crouch posture-high high-twist walk-alpha walk-beta

# Core Container Commands
up:
	touch .container_fish_history
	podman-compose up -d
shell: up
	podman exec -it jazzy_webots_sim fish

build:
	podman exec jazzy_webots_sim bash -c "source /opt/ros/jazzy/setup.bash && \
	colcon build --symlink-install --packages-select hexadrone_core hexadrone_controller hexadrone_webots"

run:
	podman exec -it jazzy_webots_sim bash -c "source install/setup.bash && \
	ros2 launch hexadrone_webots launch.py"

run-keyboard:
	podman exec -it jazzy_webots_sim bash -c "source install/setup.bash && \
	ros2 launch hexadrone_webots launch.py use_joystick:=false"

stop:
	podman-compose down -t0

# Outdated but for the backup:
# Posture Aliases (in degrees)
# **Group A (Power Group 1):** LF (0-2), RM (3-5), LB (6-8)
# **Group B (Power Group 2):** RF (9-11), LM (12-14), RB (15-17)
# Run these from a second host terminal tab while the simulation is active
# to test joint mapping and power groups.
# (might be immediately reset by an active brain cycle)

posture-prone:
	podman exec jazzy_webots_sim bash -c 'source /opt/ros/jazzy/setup.bash && \
	ros2 topic pub --once /forward_position_controller/commands std_msgs/msg/Float64MultiArray "{data: [ \
	  0.0,  40.0, -26.0, \
	  0.0,  40.0, -26.0, \
	  0.0,  40.0, -26.0, \
	  0.0, -40.0,  26.0, \
	  0.0, -40.0,  26.0, \
	  0.0, -40.0,  26.0  \
	]}"'

posture-standard:
	podman exec jazzy_webots_sim bash -c 'source /opt/ros/jazzy/setup.bash && \
	ros2 topic pub --once /forward_position_controller/commands std_msgs/msg/Float64MultiArray "{data: [ \
	  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, \
	  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  \
	]}"'

posture-crouch:
	podman exec jazzy_webots_sim bash -c 'source /opt/ros/jazzy/setup.bash && \
	ros2 topic pub --once /forward_position_controller/commands std_msgs/msg/Float64MultiArray "{data: [ \
	  0.0,  90.0,  90.0, \
	  0.0,  90.0,  90.0, \
	  0.0,  90.0,  90.0, \
	  0.0, -90.0, -90.0, \
	  0.0, -90.0, -90.0, \
	  0.0, -90.0, -90.0  \
	]}"'

posture-high:
	podman exec jazzy_webots_sim bash -c 'source /opt/ros/jazzy/setup.bash && \
	ros2 topic pub --once /forward_position_controller/commands std_msgs/msg/Float64MultiArray "{data: [ \
	  0.0, -40.0, -34.4, \
	  0.0, -40.0, -34.4, \
	  0.0, -40.0, -34.4, \
	  0.0,  40.0,  34.4, \
	  0.0,  40.0,  34.4, \
	  0.0,  40.0,  34.4  \
	]}"'

high-twist:
	podman exec jazzy_webots_sim bash -c 'source /opt/ros/jazzy/setup.bash && \
	ros2 topic pub --once /forward_position_controller/commands std_msgs/msg/Float64MultiArray "{data: [ \
	  15.0, -40.0, -34.4, \
	  15.0, -40.0, -34.4, \
	  15.0, -40.0, -34.4, \
	  15.0,  40.0,  34.4, \
	  15.0,  40.0,  34.4, \
	  15.0,  40.0,  34.4  \
	]}"'

walk-alpha:
	podman exec jazzy_webots_sim bash -c 'source /opt/ros/jazzy/setup.bash && \
	ros2 topic pub --once /forward_position_controller/commands std_msgs/msg/Float64MultiArray "{data: [ \
	  0.0, -26.0, -34.4, \
	  0.0, -26.0, -34.4, \
	  0.0, -26.0, -34.4, \
	  15.0, 40.0,  34.4, \
	  15.0, 40.0,  34.4, \
	  15.0, 40.0,  34.4  \
	]}"'

walk-beta:
	podman exec jazzy_webots_sim bash -c 'source /opt/ros/jazzy/setup.bash && \
	ros2 topic pub --once /forward_position_controller/commands std_msgs/msg/Float64MultiArray "{data: [ \
	  15.0, -40.0, -34.4, \
	  15.0, -40.0, -34.4, \
	  15.0, -40.0, -34.4, \
	  0.0,   26.0,  34.4, \
	  0.0,   26.0,  34.4, \
	  0.0,   26.0,  34.4  \
	]}"'
