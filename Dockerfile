FROM ros:rolling

ENV NVIDIA_VISIBLE_DEVICES=all \
    NVIDIA_DRIVER_CAPABILITIES=compute,utility
# Set the terminal type for colors
ENV TERM=xterm-256color

# install bootstrap tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates wget libfuse2 libasound2t64 qt6-base-dev\
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


# Enable color prompt and colorize ls/grep commands
RUN echo "export PS1='\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '" >> ~/.bashrc && \
    echo "alias ls='ls --color=auto'" >> ~/.bashrc && \
    echo "alias grep='grep --color=auto'" >> ~/.bashrc