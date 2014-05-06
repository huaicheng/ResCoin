#!/bin/bash

#相比较于mkdir,对目录是否已经存在进行检查,其余参数均相同
function fl_mkdir() #i:dir
{
    for i in $@
    do
        if [ ! -e $i ]; then
            mkdir -p $i 
        else
            #echo "can't create directory of "$i", already exists..."
            :
        fi
    done
}

function get_blkio_time()  #1: blkio.time file(e.g. "8:0 2456")
{ 
    blkio_time=$(cat $1 | awk '{print $2}')
    if [ -z "$blkio_time" ]
    then
        echo "0"
    else
        echo "$blkio_time"
    fi
}

function log() 
{
    local prefix="[$(date +%Y/%m/%d\ %H:%M:%S)]:"
    echo "${prefix} $@" >&2
}

function extract_commencts() #1: object file
{
    egrep "^#"
}

# 提取文件扩展名
function filext ()  $1
{ 
    echo ${1##*.}
}
