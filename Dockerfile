FROM mcr.microsoft.com/devcontainers/cpp:dev-debian AS compiler

WORKDIR /compiler

RUN apt update && apt install -y libboost-all-dev
RUN vcpkg install nlohmann-json
RUN vcpkg install cpr
RUN vcpkg install curl
RUN vcpkg install openssl
RUN vcpkg install libsodium

COPY ./src ./src
COPY ./api ./api
COPY ./plugins ./plugins
COPY CMakeLists.txt CMakeLists.txt

RUN cmake -DCMAKE_TOOLCHAIN_FILE=/usr/local/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -B build && \
              cmake --build build && \
              cmake --build build --target COPY_ALL_PDBS

FROM ubuntu:24.04

WORKDIR /bilibili-backend

RUN apt update && apt install -y libboost-url1.83.0 ca-certificates
COPY --from=compiler /compiler/build/Release ./

CMD ["sh","-c","stdbuf -oL -eL ./BiliBili_Reranger"]