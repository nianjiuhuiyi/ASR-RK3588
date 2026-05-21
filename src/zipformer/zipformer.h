// Copyright (c) 2024 by Rockchip Electronics Co., Ltd. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _RKNN_DEMO_ZIPFORMER_H_
#define _RKNN_DEMO_ZIPFORMER_H_

#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>

#include <spdlog/spdlog.h>
#include <rknn_api.h>

#include "audio_utils.h"
#include "process.h"

namespace ZipFormer {

#define BLANK_ID 0
#define UNK_ID 2

typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
    rknn_input *inputs;
    rknn_output *outputs;
} rknn_app_context_t;

typedef struct {
    rknn_app_context_t encoder_context;
    rknn_app_context_t decoder_context;
    rknn_app_context_t joiner_context;
} rknn_zipformer_context_t;

class ZipFormer {
public:
    ZipFormer(const std::string &encoder_path, const std::string &decoder_path, const std::string &joiner_path);
    ~ZipFormer();

    std::string run(const std::string &audio_path);

private:
    int init_zipformer_model(const std::string &model_path, rknn_app_context_t *app_ctx);
    void build_input_output(rknn_app_context_t *app_ctx);

    int inference_zipformer_model(std::vector<std::string> &recognized_text, std::vector<float> &timestamp, float &audio_length);

    int release_zipformer_model(rknn_app_context_t *app_ctx);

private:
    // std::shared_ptr<rknn_app_context_t> m_AppCtx;
    // rknn_zipformer_context_t *m_Rknn_app_ctx;
    // VocabEntry m_Vocab[VOCAB_NUM];
    // audio_buffer_t *m_Audio;

    std::shared_ptr<rknn_zipformer_context_t> m_Rknn_app_ctx;
    std::shared_ptr<audio_buffer_t> m_Audio;
    VocabEntry m_Vocab[VOCAB_NUM];

    TIMER m_Timer;
};

} // namespace ZipFormer

#endif //_RKNN_DEMO_ZIPFORMER_H_
