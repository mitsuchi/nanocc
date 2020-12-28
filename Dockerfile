FROM ubuntu:20.04

RUN apt update
RUN apt install -y gcc
RUN apt install -y make
RUN apt install -y gdb
RUN touch /root/.gdbinit
RUN echo "set history filename /root/.gdb_history" >> /root/.gdbinit
RUN echo "set history save on" >> /root/.gdbinit

ADD . /nanocc
WORKDIR /nanocc