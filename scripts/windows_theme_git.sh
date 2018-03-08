#!/bin/bash
. .install
PREFIX="${PREFIX:-`pwd`/build}"
GIT_REPO="https://github.com/kuba160/Windows-10"

git clone $GIT_REPO $PREFIX/share/themes/Windows-10

GIT_REPO="https://github.com/B00merang-Project/Windows-10-Icons"

git clone $GIT_REPO $PREFIX/share/icons/Windows-10-Icons
