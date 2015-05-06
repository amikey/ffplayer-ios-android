一、系统环境
	Win7 SP1 64位

二、Cygwin+NDK环境的配置就不多说了，可以参考很久以前写的一篇文章
	NDK开发环境搭建

三、编译ffmpeg
	用Cygwin编译FFMpeg，与在linux中有点类似，只不过，对于NDK的路径配置有点不一样。还有，最最主要的是，需要设置一下临时目录的环境变量，最初就是没设置这玩意，怎么都编译不过，才跑Linux下去玩的。

	1、下载ffmpeg2.2.5版本代码，并解压。
	2、在ffmpeg目录下创建一个config.sh脚本。
NDK=D:/Android/android-ndk-r9
PLATFORM=$NDK/platforms/android-9/arch-arm/
PREBUILT=$NDK/toolchains/arm-linux-androideabi-4.8/prebuilt/windows
CPU=armv7-a
OPTIMIZE_CFLAGS="-mfloat-abi=softfp -marm -mcpu=cortex-a8"
PREFIX=./android/$CPU
./configure --target-os=linux \
    --prefix=$PREFIX \
    --enable-cross-compile \
    --arch=arm \
    --enable-nonfree \
    --enable-asm \
    --cpu=cortex-a8 \
    --cc=$PREBUILT/bin/arm-linux-androideabi-gcc \
    --cross-prefix=$PREBUILT/bin/arm-linux-androideabi- \
    --nm=$PREBUILT/bin/arm-linux-androideabi-nm \
    --sysroot=$PLATFORM \
    --extra-cflags=" -O3 -fpic -DANDROID -DHAVE_SYS_UIO_H=1 $OPTIMIZE_CFLAGS " \
    --disable-shared \
    --enable-static \
    --extra-ldflags="-Wl,-rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib  -nostdlib -lc -lm -ldl -llog" \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffserver \
    --enable-swscale \
    --enable-swresample \
    --enable-avformat \
    --enable-avcodec \
    --disable-optimizations \
    --disable-debug \
    --disable-doc \
    --disable-stripping \
    --enable-pthreads \
    --disable-yasm \
    --enable-zlib \
    --enable-pic \
    --enable-small

make clean
make  -j4 install

$PREBUILT/bin/arm-linux-androideabi-ar d libavcodec/libavcodec.a inverse.o

$PREBUILT/bin/arm-linux-androideabi-ld -rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib  -soname libffmpeg.so -shared -nostdlib  -z noexecstack -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libffmpeg.so libavcodec/libavcodec.a libavformat/libavformat.a libavutil/libavutil.a libswscale/libswscale.a -lc -lm -lz -ldl -llog --dynamic-linker=/system/bin/linker $PREBUILT/lib/gcc/arm-linux-androideabi/4.8/libgcc.a
	NDK要换成你NDK对应的本地路径，PLATFORM和PREBUILT中的路径与你安装的NDK的版本有关。这里还要注意的一点是，不建议用记事本来创建编辑这个文件，可能用Notepad或Editplus之类的文本编辑器，因为Cygwin是模拟linux环境，linux和win的编码还是有那么点小区别的。
	
	准备工作完成后，打开Cygwin，cd到ffmpeg目录下。接下来是设置临时目录环境变量
export TMPDIR=D:/Android/cygwin/tmp
	再后面和linux一样，为config.sh赋予执行权限，然后直接./config.sh编译
Windows中用Cygwin+NDK编译FFMpeg - 过☆客 - 过☆客
 	又是一个漫长的等待时间，这个速度没法与linux下比。编译成功后，生成的库依然在ffmpeg目录下的android目录中

更多FFMPEG相关的请转至 http://zengwu3915.blog.163.com/blog/static/27834897201462210172410/