#!/bin/bash
set -e

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

SSHOPTS='-q -o StrictHostKeyChecking=no'
SSH="ssh $SSHOPTS"

VERSION=$(<"build_data/VERSION")

if [ -z $GITHUB_REF ]; then
    exit 1
fi

BRANCH_NAME=${GITHUB_REF#"refs/heads/"}
BRANCH_NAME=${BRANCH_NAME#"refs/tags/"}
echo "Ref: $GITHUB_REF"
echo "Branch/Tag: $BRANCH_NAME"

DEBUG=false

for arg in "$@"; do
    if [[ "$arg" == "--debug" ]]; then
        DEBUG=true
        break
    fi
done

REMOTE_BASE="/home/frs/project/d/de/deadbeef/Builds/$BRANCH_NAME"
REMOTE_URL="waker,deadbeef@frs.sourceforge.net"

upload_files() {
    local local_glob=$1
    local subpath=$2

    local remote_full="$REMOTE_BASE/$subpath"
    rsync -e "$SSH" $local_glob "${REMOTE_URL}:$remote_full/"
}

create_dir() {
    local subpath=$1
    local full_path="$REMOTE_BASE/$subpath"

    # Split the path into components
    IFS='/' read -r -a dirs <<< "$full_path"

    # Build SFTP commands for each folder
    local sftp_cmds=""
    local path=""
    for d in "${dirs[@]}"; do
        # Skip empty components (from leading /)
        [ -z "$d" ] && continue
        path="$path/$d"
        sftp_cmds+="mkdir $path
"
    done
    sftp_cmds+="bye
"

    # Run SFTP with the commands
    sftp $SSHOPTS "$REMOTE_URL" <<< "$sftp_cmds"
}

set -x
case "$BUILD_OS_NAME" in
    linux)
        echo Uploading linux artifacts...
        create_dir linux

        # Upload packages / source only for release build
        if [ "$DEBUG" != "true" ]; then
            upload_files "deadbeef-*.tar.bz2" linux
            upload_files "package_out/x86_64/debian/*.deb" linux
            upload_files "package_out/x86_64/arch/*.pkg.tar.xz" linux
        fi
        # This will match the build tarballs and debug symbols
        upload_files "portable_out/build/*.tar.bz2" linux
    ;;
    osx)
        echo Uploading mac artifacts...
        create_dir macOS
        upload_files "osx/build/Release/deadbeef-$VERSION-macos-universal.zip" macOS
        upload_files "osx/build/Release/deadbeef-$VERSION-macos-dSYM.zip" macOS
    ;;
    windows)
        echo Uploading windows artifacts...
        create_dir windows
        upload_files "bin/deadbeef-$VERSION-windows-x86_64.zip" windows
        upload_files "bin/deadbeef-$VERSION-windows-x86_64_DEBUG.zip" windows
        upload_files "bin/deadbeef-$VERSION-windows-x86_64.exe" windows
        upload_files "bin/deadbeef-$VERSION-windows-x86_64_DEBUG.exe" windows
        taskkill //IM ssh-agent.exe //F
    ;;
esac
