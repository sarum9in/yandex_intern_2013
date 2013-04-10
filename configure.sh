#!/bin/bash -e

absolute()
{
    if [[ ${1:0:1} = / ]]
    then
        echo "$1"
    else
        echo "$PWD/$1"
    fi
}

log()
{
    echo "$@" >&2
}

die()
{
    log "$@"
    exit 1
}

run_cmake()
(
    log "Configuring \"$2\" in \"$1\"."
    cd "$1"
    cmake "$2"
    log
)

root="$(absolute "$(dirname "$0")")"

if [[ -z $1 ]]
then
    build_path="$root/build"
else
    build_path="$(absolute "$1")"
fi

if [[ -e $build_path ]]
then
    die "Will not overwrite existing \"$build_path\", remove it manually before \"$0\"!"
fi

git submodule init
git submodule update --force --recursive

system_config()
{
    echo "set(CMAKE_BUILD_TYPE Release)"
    echo
    echo "set(root \"$root\")"
    echo "set(build_path \"$build_path\")"
    echo
    echo "set(CMAKE_MODULE_PATH \${root}/submodules/10_bunsan_common \${root}/submodules/30_yandex_contest_common)"
    echo
    echo "foreach(proj $(ls "$root/submodules"))"
    echo "    include_directories(\${root}/submodules/\${proj}/include)"
    echo "    link_directories(\${build_path}/\${proj})"
    echo "endforeach()"
}

system_config >"$root/system-config.cmake"

mkdir "$build_path"
echo ".PHONY: all" >"$build_path/Makefile"
echo "all:" >>"$build_path/Makefile"

for proj in "$root/submodules/"*
do
    proj_name="$(basename "$proj")"
    proj_build="$build_path/$proj_name"
    ln -sf "$root/system-config.cmake" "$proj/system-config.cmake"
    mkdir "$proj_build"
    run_cmake "$proj_build" "$proj"
    echo -e "\t@ \$(MAKE) \$(MFLAGS) -C \"$proj_build\"" >>"$build_path/Makefile"
done

mkdir "$build_path/root"
run_cmake "$build_path/root" "$root"
echo -e "\t@ \$(MAKE) \$(MFLAGS) -C \"$build_path/root\"" >>"$build_path/Makefile"
echo -e "\t@ ln -sf root/yandex_intern_2013_sort sort" >>"$build_path/Makefile"
