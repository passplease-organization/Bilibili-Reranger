FROM jetbrains/qodana-cpp:2025.3-eap

WORKDIR /dependencies

ARG VCPKG_COMMIT=bc994510d2eb11aac7b43b03f67a7751d5bfe0e4

RUN apt update && apt install -y build-essential pkg-config ninja-build curl zip unzip tar gcc-12 g++-12
RUN git clone https://github.com/microsoft/vcpkg.git

WORKDIR ./vcpkg
RUN git checkout ${VCPKG_COMMIT}
RUN ./bootstrap-vcpkg.sh && ./vcpkg integrate bash
ENV VCPKG_ROOT=/dependencies/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

WORKDIR /dependencies
RUN curl -LO https://cmake.org/files/v4.0/cmake-4.0.2-linux-x86_64.tar.gz && \
    tar -xzf cmake-4.0.2-linux-x86_64.tar.gz -C /opt && \
    ln -sf /opt/cmake-4.0.2-linux-x86_64/bin/cmake /usr/local/bin/cmake && \
    ln -sf /opt/cmake-4.0.2-linux-x86_64/bin/ctest /usr/local/bin/ctest && \
    ln -sf /opt/cmake-4.0.2-linux-x86_64/bin/cpack /usr/local/bin/cpack
ENV PATH="/opt/cmake-4.0.2-linux-x86_64/bin:${PATH}"
RUN which cmake && cmake --version

WORKDIR /program
RUN vcpkg install boost-url
RUN apt update && apt install -y bison flex && vcpkg install libpqxx
RUN vcpkg install nlohmann-json
RUN vcpkg install cpr
RUN vcpkg install curl
RUN vcpkg install openssl

RUN apt update && apt install -y autoconf automake libtool pkg-config autoconf-archive
RUN vcpkg install libsodium

WORKDIR /data/project