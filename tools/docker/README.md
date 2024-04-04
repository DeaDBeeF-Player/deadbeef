# Building portable build using docker

1. Make sure you have `docker` installed
2. `cd` to the root directory of this repository
3. Run once: `./tools/docker/18.04/bootstrap.sh`
4. For each build: `./tools/docker/build.sh`
5. Build artifacts are generated in `./docker-artifacts` folder

To speed up the builds, you can pre-download `static-deps`:
```
wget http://sourceforge.net/projects/deadbeef/files/staticdeps/ddb-static-deps-latest.tar.bz2/download -O ddb-static-deps.tar.bz2
mkdir -p static-deps
tar jxf ddb-static-deps.tar.bz2 -C static-deps
```
Otherwise, `build.sh` will do that automatically for each build.
