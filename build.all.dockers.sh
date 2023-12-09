#!/bin/sh
TRAVIS_BUILD_DIR=`pwd`
DOCKER_BUILD_DIR=/home/travis

docker run --user $(id -u):$(id -g) -v ${TRAVIS_BUILD_DIR}:${DOCKER_BUILD_DIR} -w ${DOCKER_BUILD_DIR} thijswithaar/debian:sid ${DOCKER_BUILD_DIR}/build.sh build.debian

docker run --user $(id -u):$(id -g) -v ${TRAVIS_BUILD_DIR}:${DOCKER_BUILD_DIR} -w ${DOCKER_BUILD_DIR} thijswithaar/ubuntu:devel ${DOCKER_BUILD_DIR}/build.sh build.ubuntu

docker run --user $(id -u):$(id -g) -v ${TRAVIS_BUILD_DIR}:${DOCKER_BUILD_DIR} -w ${DOCKER_BUILD_DIR} thijswithaar/fedora:rawhide ${DOCKER_BUILD_DIR}/build.sh build.fedora

docker run --user $(id -u):$(id -g) -v ${TRAVIS_BUILD_DIR}:${DOCKER_BUILD_DIR} -w ${DOCKER_BUILD_DIR} thijswithaar/arch:latest ${DOCKER_BUILD_DIR}/build.sh build.arch

docker run --user $(id -u):$(id -g) -v ${TRAVIS_BUILD_DIR}:${DOCKER_BUILD_DIR} -w ${DOCKER_BUILD_DIR} thijswithaar/suse:tumbleweed ${DOCKER_BUILD_DIR}/build.sh build.suse

docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
docker run --user $(id -u):$(id -g) -v ${TRAVIS_BUILD_DIR}:${DOCKER_BUILD_DIR} -w ${DOCKER_BUILD_DIR} thijswithaar/raspbian:sid ${DOCKER_BUILD_DIR}/build.sh build.raspbian

# Interactive
#docker run -it --user $(id -u):$(id -g) -v ${TRAVIS_BUILD_DIR}:${DOCKER_BUILD_DIR} -w ${DOCKER_BUILD_DIR} thijswithaar/suse:tumbleweed /bin/bash
