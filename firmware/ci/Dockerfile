FROM debian:stretch
LABEL maintainer="rundb"

ENV DEBIAN_FRONTEND=noninteractive

ARG HOST_ARCHITECTURE

RUN apt update && apt install -y wget bzip2 unzip make gcc g++ git 

ARG GCC_VERSION="gcc-arm-none-eabi-10.3-2021.10"
ARG GCC_PATH="${GCC_VERSION}"
ARG GCC_ARCHIVE_NAME_X86="${GCC_VERSION}-x86_64-linux.tar.bz2"
ARG GCC_ARCHIVE_NAME_ARM="${GCC_VERSION}-aarch64-linux.tar.bz2"

RUN mkdir /tools

RUN  if [ "${HOST_ARCHITECTURE}" = "armv7" ]; then \
      cd /tools && wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/${GCC_ARCHIVE_NAME_ARM} -P /tools \
      && tar -xf ./${GCC_ARCHIVE_NAME_ARM} && rm ${GCC_ARCHIVE_NAME_ARM}; \
   else \
      cd /tools && wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/${GCC_ARCHIVE_NAME_X86} -P /tools \
      && tar -xf ./${GCC_ARCHIVE_NAME_X86} && rm ${GCC_ARCHIVE_NAME_X86}; \
   fi

ARG CMAKE_INSTALL_SCRIPT="cmake_install_script.sh"
RUN cd /tools && mkdir cmake && cd cmake && \
   if [ "${HOST_ARCHITECTURE}" = "armv7" ]; then \
      wget -O ${CMAKE_INSTALL_SCRIPT} https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0-linux-aarch64.sh; \
   else \
      wget -O ${CMAKE_INSTALL_SCRIPT} https://github.com/Kitware/CMake/releases/download/v3.25.1/cmake-3.25.1-linux-x86_64.sh; \ 
   fi \
   && chmod +x ./${CMAKE_INSTALL_SCRIPT}  && ./${CMAKE_INSTALL_SCRIPT} --skip-license

ENV PATH="${PATH}:/tools/cmake/bin:/tools/${GCC_PATH}/bin"

# Set up Nordic instruments
ARG SDK_FOLDER_NAME="nrf5_sdk_17.1.0_ddde560"
ARG SDK_ZIP_FILE_NAME="${SDK_FOLDER_NAME}.zip"

RUN mkdir -p /sdk && cd /sdk && wget https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/${SDK_ZIP_FILE_NAME} && \
  unzip ${SDK_ZIP_FILE_NAME} -d ./ && rm ${SDK_ZIP_FILE_NAME}
