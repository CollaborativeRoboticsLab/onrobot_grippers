FROM ros:jazzy

ARG USERNAME=ubuntu

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        libmodbus-dev \
        python3-colcon-common-extensions \
        python3-pytest \
        python3-rosdep \
        python3-venv \
        python3-vcstool \
        ros-jazzy-xacro \
        sudo \
    && rm -rf /var/lib/apt/lists/* \
    && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > "/etc/sudoers.d/${USERNAME}" \
    && chmod 0440 "/etc/sudoers.d/${USERNAME}"

USER ${USERNAME}

RUN printf '%s\n' \
    'if [ -f "$HOME/colcon_ws/install/setup.bash" ]; then' \
    '    source "$HOME/colcon_ws/install/setup.bash"' \
    'else' \
    '    source /opt/ros/jazzy/setup.bash' \
    'fi' >> "$HOME/.bashrc"

WORKDIR /home/ubuntu/colcon_ws
