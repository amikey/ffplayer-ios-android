一、系统环境
	MAC OS X Mountain Lion 10.8.3、 XCode 5.1

二、编译FFMpeg
	1、下载ffmpeg2.2.5版本代码，并解压。
	2、下载并解压gas-preprocessor.pl （附件中有，zip格式，因网易博客不能上传zip后缀的文件，故加了个.rar）
		在终端中使用cp命令将它复制到 /usr/sbin/目录，并赋予可执行权限。
sudo cp -f gas-preprocessor/gas-preprocessor.pl /usr/sbin/
chmod +x /usr/sbin/gas-preprocessor.pl
	3、在ffmpeg目录下创建一个config.sh脚本
#!/bin/bash
SDKVERSION="7.1"

ARCHS="armv7 armv7s i386"

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
EXTRA_LDFLAGS="-I${DEVELOPER}/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDKVERSION}.sdk/usr/lib"
else
PLATFORM="iPhoneOS"
EXTRA_CONFIG="--arch=arm --target-os=darwin --enable-cross-compile --cpu=cortex-a9 --disable-armv5te"
EXTRA_CFLAGS="-w -arch ${ARCH}"
fi

mkdir -p "${INTERDIR}/${ARCH}"

./configure --prefix="${INTERDIR}/${ARCH}" \
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
    --sysroot="${DEVELOPER}/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDKVERSION}.sdk" \
    --cc="${DEVELOPER}/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang" \
    --as='/usr/local/bin/gas-preprocessor.pl' \
    --extra-cflags="${EXTRA_CFLAGS} -miphoneos-version-min=${SDKVERSION}" \
    --extra-ldflags="-arch ${ARCH} ${EXTRA_LDFLAGS} -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDKVERSION}.sdk -miphoneos-version-min=${SDKVERSION}" ${EXTRA_CONFIG} \
    --enable-pic \
    --extra-cxxflags="$CPPFLAGS -isysroot ${DEVELOPER}/Platforms/${PLATFORM}.platform/Developer/SDKs/${PLATFORM}${SDKVERSION}.sdk"

make && make install && make clean

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

	SDKVERSION 是XCode的版本，通过`xcode-select -print-path`来获取XCode的安装路径，ARCHS是编译的三种模式，接下来在终端中cd到ffmpeg目录，./config.sh执行就可以编译了。等到编译完成后，在ffmpeg目录下会多出一个built目录，里面分别是armv7 armv7s i386及三个合并的universal版本了。
iOS中编译FFMpeg - 过☆客 - 过☆客
 
更多FFMPEG相关的请转至 http://zengwu3915.blog.163.com/blog/static/27834897201462210172410/

参考文献：
http://www.cocoachina.com/bbs/read.php?tid=142628
http://stackoverflow.com/questions/21791325/error-while-build-ffmpeg-for-ios