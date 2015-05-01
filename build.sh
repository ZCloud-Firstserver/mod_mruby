#!/bin/sh

# ENV example
#
#   APXS_PATH_ENV='--with-apxs=/usr/local/apache/bin/apxs' APACHECTL_PATH_ENV='--with-apachectl=/usr/local/apache/bin/apachectl' sh build.sh
#

set -e

#APXS_PATH='--with-apxs=/usr/local/apache/bin/apxs'
APXS_PATH=''
if [ $APXS_PATH_ENV ]; then
    APXS_PATH=$APXS_PATH_ENV
fi

#APACHECTL_PATH='--with-apachectl=/usr/local/apache/bin/apachectl'
APACHECTL_PATH=''
if [ $APACHECTL_PATH_ENV ]; then
    APACHECTL_PATH=$APACHECTL_PATH_ENV
fi

if [ "$NUM_THREADS_ENV" != "" ]; then
    NUM_THREADS=$NUM_THREADS_ENV
else
    NUM_THREADS=$(expr `getconf _NPROCESSORS_ONLN` / 2)
    if [ $NUM_THREADS -eq "0" ]; then
        NUM_THREADS=1
    fi
fi

echo "NUM_THREADS=$NUM_THREADS"
echo "apxs="$APXS_PATH "apachectl="$APACHECTL_PATH

if [ ! -d "./mruby/src" ]; then
    echo "mruby Downloading ..."
    git submodule init
    git submodule update
    echo "mruby Downloading ... Done"
fi
cd mruby
if [ -d "./build" ]; then
    echo "mruby Cleaning ..."
    ./minirake clean
    echo "mruby Cleaning ... Done"
fi
echo "mruby building ..."
mv build_config.rb build_config.rb.orig
cp ../build_config.rb .
#rake BUILD_BIT='64'
#rake BUILD_TYPE='debug'
#rake
BUILD_TYPE='debug' ./minirake
echo "mruby building ... Done"
cd ..
echo "mod_mruby building ..."
./configure $APXS_PATH $APACHECTL_PATH
make NUM_THREADS=$NUM_THREADS -j $NUM_THREADS
echo "mod_mruby building ... Done"
echo "build.sh ... successful"

#sudo make install
#make restart
