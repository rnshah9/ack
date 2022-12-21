FROM --platform=linux/amd64 ubuntu:22.04 as builder

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y bison flex build-essential lua5.4 clang

COPY . /repo
WORKDIR /repo
RUN make -j8
RUN make install PREFIX=/install
RUN clang /repo/mayhem/fuzz_str2long.cpp -fsanitize=fuzzer,address /repo/modules/src/string/str2long.c -o /fuzz

FROM ubuntu:22.04 as package
COPY --from=builder /install /install
COPY --from=builder /fuzz /fuzz_str2long
