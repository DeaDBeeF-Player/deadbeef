# Building portable build using docker

1. Install and run docker
2. Make sure current directory is the root of the repo
3. Run once: `./tools/docker-build/docker-bootstrap.sh`
4. For each build: `./tools/docker-build/docker-build.sh`

To speed up the builds, make sure to download and unpack static-deps. Otherwise, docker-build.sh will do that automatically for each build.

```
wget http://sourceforge.net/projects/deadbeef/files/staticdeps/ddb-static-deps-latest.tar.bz2/download -O ddb-static-deps.tar.bz2
mkdir -p static-deps
tar jxf ddb-static-deps.tar.bz2 -C static-deps
```
