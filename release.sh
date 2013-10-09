#!/bin/sh

version_major=`grep '#define RIPCHECK_VERSION_MAJOR' src/ripcheck.h|sed 's/^.* \([0-9]\+\)$/\1/'`
version_minor=`grep '#define RIPCHECK_VERSION_MINOR' src/ripcheck.h|sed 's/^.* \([0-9]\+\)$/\1/'`
version_patch=`grep '#define RIPCHECK_VERSION_PATCH' src/ripcheck.h|sed 's/^.* \([0-9]\+\)$/\1/'`
version_release=$1
version="$version_major.$version_minor.$version_patch"

if [ "$version_release" != "" ]; then
	version="$version-$version_release"
fi

test -d "ripcheck-$version" && rm -r "ripcheck-$version"
mkdir -p "ripcheck-$version" || exit 1
git clone . "ripcheck-$version/source" || exit 1
mv "ripcheck-$version/source/README.md" "ripcheck-$version" || exit 1
rm -rf "ripcheck-$version/source/.git" || exit 1

for arch in linux64 linux32 mingw32 mingw64; do
	realarch=`echo "$arch"|sed 's/^mingw/win/'`
	echo
	echo "================================================================================"
	echo "BUILDING $realarch"
	echo "================================================================================"
	cmake_args="-DCMAKE_TOOLCHAIN_FILE=`readlink -f contrib/toolchain-$arch.cmake`"
	if [ "$MINGW64_PATH" != "" ]; then
		cmake_args="$cmake_args -DMINGW64_PATH=$MINGW64_PATH"
	fi
	case "$realarch" in
	linux64)
		cmake_args=""
		;;
	win64)
		cmake_args="$cmake_args -DWITH_VISUALIZE=OFF"
		;;
	esac

	test -d "build-$realarch" && rm -r "build-$realarch"
	mkdir "build-$realarch" || exit 1
	cd "build-$realarch" && cmake .. $cmake_args && make -j2 && cd .. || exit 1

	mkdir "ripcheck-$version/$realarch" || exit 1

	case "$realarch" in
	win*)
		exe_suffix=".exe"
		;;
	*)
		exe_suffix=""
		;;
	esac
	cp "build-$realarch/src/ripcheck$exe_suffix" "ripcheck-$version/$realarch" || exit 1

	if [ "$realarch" = "win32" ]; then
		cp contrib/zlib/zlib1.dll contrib/libpng12.dll "ripcheck-$version/win32" || exit 1
	fi
done

zip -r9 "ripcheck-$version.zip" "ripcheck-$version" || exit 1

echo "================================================================================"
echo "created release archive ripcheck-$version.zip"
echo "================================================================================"
