FROM ubuntu:20.04

RUN apt update
RUN apt install -y gcc
RUN apt install make

ADD . /nanocc
WORKDIR /nanocc