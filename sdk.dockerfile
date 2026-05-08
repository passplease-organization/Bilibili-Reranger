FROM mcr.microsoft.com/devcontainers/cpp:dev-debian AS sdk-base

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"
ENV VCPKG_ROOT=/usr/local/vcpkg

COPY .devcontainer/reinstall-cmake.sh /tmp/reinstall-cmake.sh

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}"; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        autoconf \
        autoconf-archive \
        automake \
        bison \
        ca-certificates \
        flex \
        libboost-all-dev \
        libtool \
        ninja-build \
        pkg-config \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN vcpkg install \
        cpr \
        curl \
        libpqxx \
        libsodium \
        nlohmann-json \
        openssl \
        toml11

FROM sdk-base AS sdk-builder

WORKDIR /source

COPY . .

RUN cmake --preset debug \
    && cmake --build --preset debug \
    && cmake --build build/debug --target COPY_ALL_DLLS \
    && cmake --install build/debug --prefix /opt/bilibili-reranger-sdk/debug \
    && mkdir -p /opt/bilibili-reranger-backend/debug \
    && cp -a build/debug/Debug/. /opt/bilibili-reranger-backend/debug/ \
    && cmake --preset release \
    && cmake --build --preset release \
    && cmake --build build/release --target COPY_ALL_DLLS \
    && cmake --install build/release --prefix /opt/bilibili-reranger-sdk/release \
    && mkdir -p /opt/bilibili-reranger-backend/release \
    && cp -a build/release/Release/. /opt/bilibili-reranger-backend/release/

FROM sdk-base AS sdk-artifacts

COPY --from=sdk-builder /opt/bilibili-reranger-sdk /bilibili-reranger-sdk
COPY --from=sdk-builder /opt/bilibili-reranger-backend /bilibili-reranger-backend

FROM sdk-base AS plugin-dev

COPY --from=sdk-builder /opt/bilibili-reranger-sdk /opt/bilibili-reranger-sdk
COPY --from=sdk-builder /opt/bilibili-reranger-backend /opt/bilibili-reranger-backend

ENV BILIBILI_RERANGER_SDK=/opt/bilibili-reranger-sdk/release
ENV BILIBILI_RERANGER_SDK_RELEASE=/opt/bilibili-reranger-sdk/release
ENV BILIBILI_RERANGER_SDK_DEBUG=/opt/bilibili-reranger-sdk/debug
ENV BILIBILI_RERANGER_BACKEND=/opt/bilibili-reranger-backend/release
ENV BILIBILI_RERANGER_BACKEND_RELEASE=/opt/bilibili-reranger-backend/release
ENV BILIBILI_RERANGER_BACKEND_DEBUG=/opt/bilibili-reranger-backend/debug

WORKDIR /workspace
