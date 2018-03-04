FROM deadbeef-builder-player

WORKDIR /usr/src/deadbeef
COPY . /usr/src/deadbeef
ENV TRAVIS_OS_NAME=linux

CMD [ "bash", "travis/build.sh" ]
