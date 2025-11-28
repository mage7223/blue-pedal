# Arduino Development DevContainer
FROM ubuntu:22.04

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

# Configure apt and install packages
RUN apt-get update \
    && apt-get -y install --no-install-recommends \
        # Essential development tools
        build-essential \
        git \
        curl \
        wget \
        unzip \
        ca-certificates \
        software-properties-common \
        # Python and pip (needed for ESP32 tools)
        python3 \
        python3-pip \
        python3-venv \
        # Serial communication tools
        screen \
        minicom \
        picocom \
        # USB and device utilities
        usbutils \
        # Additional utilities
        nano \
        vim \
        tree \
        jq \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user
ARG USERNAME=arduino
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Add user to dialout group for serial port access
RUN usermod -a -G dialout $USERNAME

# Switch to non-root user
USER $USERNAME
WORKDIR /home/$USERNAME

# Install Arduino CLI
ENV ARDUINO_CLI_VERSION=0.35.1
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh \
    && sudo mv bin/arduino-cli /usr/local/bin/ \
    && arduino-cli version

# Create Arduino CLI config directory and initialize
RUN mkdir -p .arduino15 \
    && arduino-cli config init

# Install ESP32 board package and required tools
RUN arduino-cli core update-index \
    && arduino-cli core install esp32:esp32 \
    && arduino-cli lib update-index

# Install commonly used Arduino libraries for ESP32 BLE projects
RUN arduino-cli lib install "ESP32 BLE Arduino"

# Install PlatformIO (alternative development environment)
RUN python3 -m pip install --user platformio \
    && echo 'export PATH=$PATH:~/.local/bin' >> ~/.bashrc

# Install esptool for ESP32 flashing
RUN python3 -m pip install --user esptool

# Create workspace directory
RUN mkdir -p /home/$USERNAME/workspace

# Set up Arduino CLI board manager URLs for ESP32
RUN arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json \
    && arduino-cli config add board_manager.additional_urls https://jihulab.com/esp-mirror/espressif/arduino-esp32.git \
    && arduino-cli core update-index 


# Install ESP32 specific libraries that might be useful
RUN arduino-cli lib install "ArduinoJson" \
    && arduino-cli lib install "WiFi" \
    && arduino-cli lib install "Preferences"

# Create useful aliases and functions
RUN echo 'alias ll="ls -la"' >> ~/.bashrc \
    && echo 'alias la="ls -A"' >> ~/.bashrc \
    && echo 'alias l="ls -CF"' >> ~/.bashrc \
    && echo 'export PATH=$PATH:~/.local/bin' >> ~/.bashrc

# Set environment variables for Arduino development
ENV ARDUINO_DIRECTORIES_USER=/home/$USERNAME/.arduino15
ENV ARDUINO_DIRECTORIES_DATA=/home/$USERNAME/.arduino15

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=dialog

# Default command
CMD ["/bin/bash"]