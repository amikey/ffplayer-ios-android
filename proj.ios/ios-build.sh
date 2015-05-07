#!/bin/bash
SDKVERSION="8.1"

#ARCHS="armv7 armv7s i386"
ARCHS="armv64"

DEVELOPER=`xcode-select -print-path`

cd "`dirname \"$0\"`"
REPOROOT=$(pwd)

# where we will store intermediary builds
INTERDIR="${REPOROOT}/built"
mkdir -p $INTERDIR

########################################
# Exit the script if an error happens

for ARCH in ${ARCHS}
do
if [ "${ARCH}" == "i386" ];
then
PLATFORM="iPhoneSimulator"
EXTRA_CONFIG="--arch=i386 --disable-asm --enable-cross-compile --target-os=darwin --cpu=i386"
EXTRA_CFLAGS="-arch i386"
EXTRA_LDFLAGS="-arch i386 -I${DEVELOPER}/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDKVERSION}.sdk/usr/lib"
else
PLATFORM="iPhoneOS"
EXTRA_CONFIG="--arch=${ARCH} --target-os=darwin --enable-cross-compile --cpu=cortex-a9 --disable-armv5te"
EXTRA_CFLAGS="-w -arch ${ARCH}"
EXTRA_LDFLAGS="-arch ${ARCH}"
if [ "$ARCH}" == "arm64" ]
then
	EXPORT="GASPP_FIX_XCODE5=1"
fi
fi

mkdir -p "${INTERDIR}/${ARCH}"

echo "==========================================================="
echo " configure "${ARCH}
echo "${INTERDIR}/${ARCH}"
echo "==========================================================="

./configure --prefix="${INTERDIR}/${ARCH}" \
    --disable-yasm \
    --disable-neon \
    --disable-armv6 \
    --disable-armv6t2 \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffserver \
    --disable-iconv \
    --disable-bzlib \
    --enable-avresample \
    --disable-shared \
    --enable-static \
    --enable-protocol=http \
    --sysroot="${DEVELOPER}/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDKVERSION}.sdk" \
    --cc="${DEVELOPER}/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang" \
    --extra-cflags="${EXTRA_CFLAGS} -miphoneos-version-min=${SDKVERSION}" \
    --extra-ldflags="${EXTRA_LDFLAGS} -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDKVERSION}.sdk -miphoneos-version-min=${SDKVERSION}" ${EXTRA_CONFIG} \
    --enable-pic \
    --extra-cxxflags="$CPPFLAGS -isysroot ${DEVELOPER}/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDKVERSION}.sdk"

echo "==========================================================="
echo " make "${ARCH}
echo "${INTERDIR}/${ARCH}"
echo "==========================================================="

make

echo "==========================================================="
echo " install "${ARCH}
echo "${INTERDIR}/${ARCH}"
echo "==========================================================="

make install 

echo "==========================================================="
echo " clean "${ARCH}
echo "${INTERDIR}/${ARCH}"
echo "==========================================================="

make clean

done

mkdir -p "${INTERDIR}/universal/lib"

cd "${INTERDIR}/armv7/lib"
for file in *.a
do

cd ${INTERDIR}
xcrun -sdk iphoneos lipo -output universal/lib/$file  -create -arch armv7 armv7/lib/$file -arch armv7s armv7s/lib/$file -arch i386 i386/lib/$file
echo "Universal $file created."

done
cp -r ${INTERDIR}/armv7/include ${INTERDIR}/universal/

echo "Done."
