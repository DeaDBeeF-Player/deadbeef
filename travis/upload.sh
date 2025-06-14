#!/bin/bash
echo Decrypting id_ed25519...

mkdir -p sshconfig

if [ ! -z $gh_ed25519_key ]; then
    openssl aes-256-cbc -K $gh_ed25519_key -iv $gh_ed25519_iv -in .github/id_ed25519.enc -out sshconfig/id_ed25519 -d || exit 1
else
    echo "SSH key is not available, upload cancelled"
    exit 0
fi

eval "$(ssh-agent -s)"
chmod 600 sshconfig/id_ed25519
ssh-add sshconfig/id_ed25519 || exit 1

SSHOPTS="ssh -o StrictHostKeyChecking=no"

VERSION=`tr -d '\r' < PORTABLE_VERSION`

if [ ! -z $GITHUB_REF ]; then
    TRAVIS_BRANCH=${GITHUB_REF#"refs/heads/"}
    TRAVIS_BRANCH=${TRAVIS_BRANCH#"refs/tags/"}
    echo "Ref: $GITHUB_REF"
    echo "Branch/Tag: $TRAVIS_BRANCH"
fi

case "$TRAVIS_OS_NAME" in
    linux)
        echo Uploading linux artifacts...
        rsync -e "$SSHOPTS" deadbeef-*.tar.bz2 waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/linux/$TRAVIS_BRANCH/ || exit 1
        rsync -e "$SSHOPTS" portable_out/build/*.tar.bz2 waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/linux/$TRAVIS_BRANCH/ || exit 1
        rsync -e "$SSHOPTS" package_out/x86_64/debian/*.deb waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/linux/$TRAVIS_BRANCH/ || exit 1
        rsync -e "$SSHOPTS" package_out/x86_64/arch/*.pkg.tar.xz waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/linux/$TRAVIS_BRANCH/ || exit 1
    ;;
    osx)
        echo Uploading mac artifacts...
        rsync -e "$SSHOPTS" osx/build/Release/deadbeef-$VERSION-macos-universal.zip waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/macOS/$TRAVIS_BRANCH/ || exit 1
        rsync -e "$SSHOPTS" osx/build/Release/deadbeef-$VERSION-macos-dSYM.zip waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/macOS/$TRAVIS_BRANCH/ || exit 1
    ;;
    windows)
        echo Uploading windows artifacts...
        rsync -e "$SSHOPTS" bin/deadbeef-$VERSION-windows-x86_64.zip waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/windows/$TRAVIS_BRANCH/ || exit 1
        rsync -e "$SSHOPTS" bin/deadbeef-$VERSION-windows-x86_64_DEBUG.zip waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/windows/$TRAVIS_BRANCH/ || exit 1
        rsync -e "$SSHOPTS" bin/deadbeef-$VERSION-windows-x86_64.exe waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/windows/$TRAVIS_BRANCH/ || exit 1
        rsync -e "$SSHOPTS" bin/deadbeef-$VERSION-windows-x86_64_DEBUG.exe waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/windows/$TRAVIS_BRANCH/ || exit 1
        taskkill //IM ssh-agent.exe //F
    ;;
esac
