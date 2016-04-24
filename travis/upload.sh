echo Decrtypting id_rsa...
openssl aes-256-cbc -K $encrypted_b1899526f957_key -iv $encrypted_b1899526f957_iv -in travis/id_rsa.enc -out travis/id_rsa -d || exit 1
eval "$(ssh-agent -s)"
chmod 600 travis/id_rsa
ssh-add travis/id_rsa || exit 1

SSHOPTS="ssh -o StrictHostKeyChecking=no"

VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`

if [[ $VERSION =~ ^[0-9]*\.[0-9]*\.[0-9]$ ]]
then
    # release, upload to root folder
    echo Uploading to frs.sourceforge.net:/home/frs/project/d/de/deadbeef/
    UPLOAD_PATH=waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/
else
    # not release, upload to travis folder
    echo Uploading to /home/frs/project/d/de/deadbeef/travis/linux/$TRAVIS_BRANCH/
    UPLOAD_PATH=waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/linux/$TRAVIS_BRANCH/
fi

case "$TRAVIS_OS_NAME" in
    linux)
        echo Uploading linux artifacts...
        rsync -e "$SSHOPTS" deadbeef-*.tar.bz2 $UPLOAD_PATH || exit 1
        rsync -e "$SSHOPTS" portable_out/build/*.tar.bz2 $UPLOAD_PATH || exit 1
        if [[ $VERSION =~ ^[0-9]*\.[0-9]*\.[0-9]$ ]]
        then
            ./scripts/packages_upload.sh || exit 1
        fi
    ;;
    osx)
        echo Uploading mac artifacts...
        rsync -e "$SSHOPTS" osx/build/Release/deadbeef-$VERSION-osx-x86_64.zip waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/travis/osx/$TRAVIS_BRANCH/ || exit 1
    ;;
esac
