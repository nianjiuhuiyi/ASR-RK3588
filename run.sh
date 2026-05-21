#!/usr/bin/env bash

set -e

model_path=./resources/model

if (($# == 0))
then
    nohup ./asr_rknn_serve ${model_path}/encoder-epoch-99-avg-1.rknn \
                        ${model_path}/decoder-epoch-99-avg-1.rknn \
                        ${model_path}/joiner-epoch-99-avg-1.rknn \
            > /dev/null 2>&1 & 
    echo -e "\n服务已启动，请查看日志：tail -f logs/zipformer/zipformer.log\n"
    
elif [[ $# == 1 ]]
then
    audio_file=$1
    ./asr_rknn_serve ${model_path}/encoder-epoch-99-avg-1.rknn \
                        ${model_path}/decoder-epoch-99-avg-1.rknn \
                        ${model_path}/joiner-epoch-99-avg-1.rknn \
                        $audio_file

else
    echo -e "参数错误，请参考README.\n"
fi
