FROM mcr.microsoft.com/devcontainers/cpp:dev-debian AS compiler

WORKDIR /compiler

RUN apt update && apt install -y libboost-all-dev
RUN apt install -y bison flex && vcpkg install libpqxx
RUN vcpkg install nlohmann-json
RUN vcpkg install cpr
RUN vcpkg install curl
RUN vcpkg install openssl

RUN apt update && apt install -y autoconf automake libtool pkg-config autoconf-archive
RUN vcpkg install libsodium

COPY ./src ./src
COPY ./api ./api
COPY ./plugins ./plugins
COPY CMakeLists.txt CMakeLists.txt
COPY CMakePresets.json CMakePresets.json

RUN cmake --preset release && \
    cmake --build --preset release && \
    cmake --build build/release --target COPY_ALL_DLLS

FROM ubuntu:24.04

WORKDIR /bilibili-backend

RUN apt update && apt install -y libboost-url1.83.0 ca-certificates
COPY --from=compiler /compiler/build/release/Release ./

CMD ["sh","-c","stdbuf -oL -eL ./BiliBili_Reranger"]