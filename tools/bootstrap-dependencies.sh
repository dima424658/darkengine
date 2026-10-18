#!/usr/bin/env sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repository_root=$(CDPATH= cd -- "$script_dir/.." && pwd)
dependency_root=$(CDPATH= cd -- "$repository_root/../.." && pwd)/darkengine-deps
target=${1-all}

if [ "$#" -gt 1 ] || { [ "$target" != all ] && [ "$target" != game ] && [ "$target" != dromed ]; }; then
    echo "usage: $0 [all|game|dromed]" >&2
    exit 2
fi

install_git_dependency() {
    name=$1
    repository=$2
    tag=$3
    required_file=$4
    destination=$dependency_root/$name

    if [ -f "$destination/$required_file" ]; then
        echo "$name is already available at $destination"
        return
    fi
    if [ -e "$destination" ]; then
        echo "$destination exists but does not contain $required_file" >&2
        exit 1
    fi

    mkdir -p "$dependency_root"
    git clone --branch "$tag" --depth 1 "$repository" "$destination"
    if [ ! -f "$destination/$required_file" ]; then
        echo "failed to install $name $tag at $destination" >&2
        exit 1
    fi
}

if [ "$target" = all ] || [ "$target" = game ]; then
    install_git_dependency squirrel \
        https://github.com/albertodemichelis/squirrel.git v3.2 \
        include/squirrel.h
fi

if [ "$target" = all ] || [ "$target" = dromed ]; then
    install_git_dependency kctools \
        https://github.com/JarrodDoyle/KCTools.git v1.3.4 \
        KCTools.slnx
fi
