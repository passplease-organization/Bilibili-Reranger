FROM mcr.microsoft.com/devcontainers/cpp:dev-debian

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Optionally install the cmake for vcpkg
COPY reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>

RUN apt update && apt install -y libboost-all-dev
RUN apt install -y bison flex && vcpkg install libpqxx
RUN vcpkg install nlohmann-json
RUN vcpkg install cpr
RUN vcpkg install curl
RUN vcpkg install openssl
RUN vcpkg install toml11

RUN apt update && apt install -y autoconf automake libtool pkg-config autoconf-archive
RUN vcpkg install libsodium

RUN apt-get update && apt-get install -y curl \
    && curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && npm install -g @google/gemini-cli \
    && npm install -g @openai/codex \
    && npm install -g @qwen-code/qwen-code@latest \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN npm install -g @playwright/cli