FROM debian:bookworm-slim

RUN apt update \
    && apt install -y build-essential git curl zip unzip tar pkg-config python3

# setup vcpkg
WORKDIR /vcpkg

RUN git clone https://github.com/microsoft/vcpkg.git \
    && cd vcpkg && sh bootstrap-vcpkg.sh

WORKDIR /server
RUN /vcpkg/vcpkg/vcpkg install google-cloud-cpp[core,speech,texttospeech]

RUN apt install -y cmake libssl-dev pkg-config autoconf libtool libcurl4-openssl-dev

RUN git clone --recurse-submodules -b v1.62.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

RUN cd grpc \
    && mkdir -p cmake/build \
    && cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
    && make -j 4 \
    && make install 
    # && cd .. && rm -rf grpc


WORKDIR /third-party

# install curlpp
RUN git clone https://github.com/jpbarrette/curlpp \
    && cd curlpp \
    && cmake . \
    && make && make install \
    && cd .. && rm -rf curlpp

WORKDIR /server


# ADD . /server
ENV GOOGLE_APPLICATION_CREDENTIALS=/server/auth/extra-401922-75fb0086b48b.json

ENV LD_LIBRARY_PATH=/usr/local/lib
ENV GRPC_DNS_RESOLVER=native

# CMD rm -rf build && cmake -S . -B .build -DCMAKE_TOOLCHAIN_FILE=cmake -S . -B .build -DCMAKE_TOOLCHAIN_FILE=/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++\
#     && cmake --build .build && .build/server

CMD tail -f /dev/null

#git clone --recurse-submodules -b v1.41.0 https://github.com/grpc/grpc
#git clone https://github.com/protocolbuffers/protobuf.git
#docker run -v .:/server -p 8080:8080 sa:test