FROM deadbeef-builder-player-clang-14.04

WORKDIR /usr/src/deadbeef
COPY . /usr/src/deadbeef
ENV BUILD_OS_NAME=linux

RUN rm -rf m4
RUN rm ltmain.sh
CMD [ "bash", "ci_scripts/build.sh", "--clang" ]
