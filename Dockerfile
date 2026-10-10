FROM ros:rolling

# NVIDIA and GPU passthrough
ENV NVIDIA_VISIBLE_DEVICES=all \
    NVIDIA_DRIVER_CAPABILITIES=all \
    __NV_PRIME_RENDER_OFFLOAD=1 \
    __GLX_VENDOR_LIBRARY_NAME=nvidia

# Fix UI warnings and add games to PATH
ENV NO_AT_BRIDGE=1 \
    PATH="/usr/games:${PATH}"

# Set the terminal type for colors
ENV TERM=xterm-256color

# Install all dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    flightgear \
    libcanberra-gtk3-module \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    ca-certificates wget libfuse2 libasound2t64 qt6-base-dev \
    x11-utils \
    libglvnd0 libgl1 libglx0 \
    dbus dbus-x11 \
    tmux \
    libfmt-dev libspdlog-dev libopencv-dev git && \
    . /etc/os-release && \
    case "$(dpkg --print-architecture)" in \
        amd64) cuda_arch=x86_64 ;; \
        arm64) cuda_arch=sbsa ;; \
        *) echo "Unsupported architecture for CUDA: $(dpkg --print-architecture)" >&2; exit 1 ;; \
    esac && \
    cuda_repo="ubuntu$(printf '%s' "$VERSION_ID" | tr -d .)" && \
    wget -q "https://developer.download.nvidia.com/compute/cuda/repos/${cuda_repo}/${cuda_arch}/cuda-keyring_1.1-1_all.deb" \
        -O /tmp/cuda-keyring.deb && \
    dpkg -i /tmp/cuda-keyring.deb && \
    rm /tmp/cuda-keyring.deb && \
    apt-get update && \
    apt-get install -y --no-install-recommends cuda-toolkit && \
    rm -rf /var/lib/apt/lists/*

# D-Bus for FlightGear Qt launcher by wrapping the executable
RUN mv /usr/games/fgfs /usr/games/fgfs.real && \
    echo '#!/bin/bash\nexec dbus-launch /usr/games/fgfs.real "$@"' > /usr/games/fgfs && \
    chmod +x /usr/games/fgfs

# Enable color prompt and colorize ls/grep commands
RUN echo "export PS1='\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '" >> ~/.bashrc && \
    echo "alias ls='ls --color=auto'" >> ~/.bashrc && \
    echo "alias grep='grep --color=auto'" >> ~/.bashrc