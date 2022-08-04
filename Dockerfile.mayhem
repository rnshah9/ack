FROM --platform=linux/amd64 ubuntu:22.04 as builder

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y bison flex build-essential lua5.4

COPY . /repo
WORKDIR /repo
RUN make -j8
RUN make install PREFIX=/install

FROM ubuntu:22.04 as package
COPY --from=builder /install /install
