FROM deadbeef-builder-player

WORKDIR /usr/src/deadbeef
COPY . /usr/src/deadbeef
ENV TRAVIS_OS_NAME=linux

RUN rm -rf m4
RUN rm ltmain.sh
CMD [ "bash", "travis/build.sh" ]
