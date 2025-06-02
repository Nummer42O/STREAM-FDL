FROM docker.io/ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN set -x && \
  apt update -y -qq && \
  apt install -y -qq \
    sudo \
    build-essential g++ \
    pkg-config cmake \
    gdb \
    git \
    wget curl \
    zip unzip tar \
    net-tools iputils-ping \
    nano \
    python3-pip python3-venv \
    dotnet-runtime-6.0
RUN set -x && \
  apt update -y -qq && \
  apt install -y -qq \
    libeigen3-dev
RUN set -x && \
  apt update -y -qq && \
  apt install -y -qq \
    libyaml-cpp-dev
RUN set -x && \
  apt-get autoremove -y -qq

# non-root user support
ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME
RUN useradd --uid $USER_UID --gid $USER_GID -m $USERNAME

RUN set -x && \
  echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
  chmod 0440 /etc/sudoers.d/$USERNAME

# custom installs
# ARG NTHREADS=""

ENV NVIDIA_VISIBLE_DEVICES="${NVIDIA_VISIBLE_DEVICES:-all}"
ENV NVIDIA_DRIVER_CAPABILITIES="${NVIDIA_DRIVER_CAPABILITIES:+$NVIDIA_DRIVER_CAPABILITIES,}graphics"

# Set the default user. Omit if you want to keep the default as root.
USER $USERNAME
ENV USER=${USERNAME}

# convenience
ENV LD_LIBRARY_PATH="/usr/local/lib"
RUN set -x && \
  echo "alias x='exit'" >> ~/.bash_aliases && \
  echo "alias c='clear'" >> ~/.bash_aliases

WORKDIR /home/${USERNAME}/
CMD ["bash"]