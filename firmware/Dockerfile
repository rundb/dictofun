FROM debian:stretch
LABEL maintainer="rundb"

ENV DEBIAN_FRONTEND=noninteractive

# TODO: consider adding here a selection of x86/ARM targets
RUN apt update && apt install -y wget bzip2 unzip

ARG GCC_VERSION="gcc-arm-none-eabi-10.3-2021.10"
ARG GCC_PATH="${GCC_VERSION}"
ARG GCC_ARCHIVE_NAME="${GCC_VERSION}-x86_64-linux.tar.bz2"
RUN mkdir /tools && cd /tools && wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/${GCC_ARCHIVE_NAME} -P /tools\
   && tar -xf ./${GCC_ARCHIVE_NAME} && rm ${GCC_ARCHIVE_NAME}

ARG CMAKE_INSTALL_SCRIPT="cmake_install_script.sh"
RUN cd /tools && mkdir cmake && cd cmake && wget -O ${CMAKE_INSTALL_SCRIPT} https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0-linux-x86_64.sh \
   && chmod +x ./${CMAKE_INSTALL_SCRIPT}  && ./${CMAKE_INSTALL_SCRIPT} --skip-license

ENV PATH="${PATH}:/tools/cmake/bin:/tools/${GCC_PATH}/bin"

# Set up Nordic instruments
ARG SDK_FOLDER_NAME="nRF5_SDK_17.1.0_ddde560"
ARG SDK_ZIP_FILE_NAME="${SDK_FOLDER_NAME}.zip"

RUN mkdir -p /sdk && cd /sdk && wget https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5/Binaries/${SDK_ZIP_FILE_NAME} && \
  unzip ${SDK_ZIP_FILE_NAME} -d ./ && rm ${SDK_ZIP_FILE_NAME}
