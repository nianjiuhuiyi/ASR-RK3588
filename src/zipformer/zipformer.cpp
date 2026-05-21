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

#include "zipformer.h"
#include "process.h"

namespace ZipFormer {

void dump_tensor_attr(rknn_tensor_attr *attr) {
    char dims_str[100];
    char temp_str[100];
    memset(dims_str, 0, sizeof(dims_str));
    for (int i = 0; i < attr->n_dims; i++) {
        strcpy(temp_str, dims_str);
        if (i == attr->n_dims - 1) {
            sprintf(dims_str, "%s%d", temp_str, attr->dims[i]);
        }
        else {
            sprintf(dims_str, "%s%d, ", temp_str, attr->dims[i]);
        }
    }

    // printf("  index=%d, name=%s, n_dims=%d, dims=[%s], n_elems=%d, size=%d, "
    //        "fmt=%s, type=%s, qnt_type=%s, zp=%d, scale=%f\n",
    //        attr->index, attr->name, attr->n_dims, dims_str, attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
    spdlog::debug("  index={}, name={}, n_dims={}, dims=[{}], n_elems={}, size={}, "
                  "fmt={}, type={}, qnt_type={}, zp={}, scale={}\n",
                  attr->index, attr->name, attr->n_dims, dims_str, attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type), get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

void release_input_output(rknn_app_context_t *app_ctx) {
    for (int i = 0; i < app_ctx->io_num.n_input; i++) {
        if (app_ctx->inputs[i].buf != NULL) {
            free(app_ctx->inputs[i].buf);
            app_ctx->inputs[i].buf = NULL;
        }
    }

    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        if (app_ctx->outputs[i].buf != NULL) {
            free(app_ctx->outputs[i].buf);
            app_ctx->outputs[i].buf = NULL;
        }
    }

    if (app_ctx->inputs != NULL) {
        free(app_ctx->inputs);
        app_ctx->inputs = NULL;
    }

    if (app_ctx->outputs != NULL) {
        free(app_ctx->outputs);
        app_ctx->outputs = NULL;
    }
}

int inference_encoder_model(rknn_app_context_t *app_ctx) {
    int ret = 0;

    do {
        // Set Input Data
        ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, app_ctx->inputs);
        if (ret < 0) {
            spdlog::error("rknn_input_set fail! ret={}\n", ret);
            break;
        }

        // Run
        ret = rknn_run(app_ctx->rknn_ctx, nullptr);
        if (ret < 0) {
            spdlog::error("rknn_run fail! ret={}\n", ret);
            break;
        }

        // Get Output
        ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, app_ctx->outputs, NULL);
        if (ret < 0) {
            spdlog::error("rknn_outputs_get fail! ret={}\n", ret);
            break;
        }

        for (int i = 1; i < app_ctx->io_num.n_input; i++) {
            if (app_ctx->input_attrs[i].fmt == RKNN_TENSOR_NHWC) {
                int N = app_ctx->input_attrs[i].dims[0];
                int H = app_ctx->input_attrs[i].dims[1];
                int W = app_ctx->input_attrs[i].dims[2];
                int C = app_ctx->input_attrs[i].dims[3];
                convert_nchw_to_nhwc((float *)app_ctx->outputs[i].buf, (float *)app_ctx->inputs[i].buf, N, C, H, W);
            }
            else {
                memcpy(app_ctx->inputs[i].buf, app_ctx->outputs[i].buf, app_ctx->inputs[i].size);
            }
        }

    } while (0);

    return ret;
}

int inference_decoder_model(rknn_app_context_t *app_ctx) {
    int ret = 0;

    do {
        // Set Input Data
        ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, app_ctx->inputs);
        if (ret < 0) {
            spdlog::error("rknn_input_set fail! ret={}\n", ret);
            break;
        }

        // Run
        ret = rknn_run(app_ctx->rknn_ctx, nullptr);
        if (ret < 0) {
            spdlog::error("rknn_run fail! ret={}\n", ret);
            break;
        }

        // Get Output
        ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, app_ctx->outputs, NULL);
        if (ret < 0) {
            spdlog::error("rknn_outputs_get fail! ret={}\n", ret);
            break;
        }
    } while (0);

    return ret;
}

int inference_joiner_model(rknn_app_context_t *app_ctx, float *cur_encoder_output, float *decoder_output) {
    int ret = 0;

    do {
        // Set Input Data
        memcpy(app_ctx->inputs[0].buf, cur_encoder_output, app_ctx->input_attrs[0].n_elems * sizeof(float));
        memcpy(app_ctx->inputs[1].buf, decoder_output, app_ctx->input_attrs[1].n_elems * sizeof(float));
        ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, app_ctx->inputs);
        if (ret < 0) {
            printf("rknn_input_set fail! ret=%d\n", ret);
            break;
        }

        // Run
        ret = rknn_run(app_ctx->rknn_ctx, nullptr);
        if (ret < 0) {
            printf("rknn_run fail! ret=%d\n", ret);
            break;
        }

        // Get Output
        ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, app_ctx->outputs, NULL);
        if (ret < 0) {
            printf("rknn_outputs_get fail! ret=%d\n", ret);
            break;
        }
    } while (0);

    return ret;
}

int greedy_search(rknn_zipformer_context_t *app_ctx, float *encoder_output, float *decoder_output, int64_t *hyp, float *joiner_output, VocabEntry *vocab, std::vector<std::string> &recognized_text, std::vector<float> &timestamp, int num_processed_frames, int &frame_offset) {
    int ret = 0;

    ret = inference_encoder_model(&app_ctx->encoder_context);
    if (ret < 0) {
        spdlog::error("inference_encoder_model fail! ret={}\n", ret);
        return ret;
    }

    if (num_processed_frames == 0) {
        ret = inference_decoder_model(&app_ctx->decoder_context);
        if (ret < 0) {
            spdlog::error("inference_decoder_model fail! ret={}\n", ret);
            return ret;
        }
    }

    for (int i = 0; i < ENCODER_OUTPUT_T; i++) {
        float *cur_encoder_output = encoder_output + i * DECODER_DIM;
        ret = inference_joiner_model(&app_ctx->joiner_context, cur_encoder_output, decoder_output);
        if (ret < 0) {
            spdlog::error("inference_joiner_model fail! ret={}\n", ret);
            return ret;
        }

        int next_token = argmax(joiner_output);
        if (next_token != BLANK_ID && next_token != UNK_ID) {
            timestamp.push_back(frame_offset + i);

            for (int j = 0; j < CONTEXT_SIZE - 1; j++) {
                hyp[j] = hyp[j + 1];
            }

            hyp[CONTEXT_SIZE - 1] = (int64_t)next_token;
            std::string next_token_str = vocab[next_token].token;
            replace_substr(next_token_str, "▁", " ");
            recognized_text.push_back(next_token_str);
            ret = inference_decoder_model(&app_ctx->decoder_context);
            if (ret < 0) {
                spdlog::error("inference_decoder_model fail! ret={}\n", ret);
                return ret;
            }
        }
    }

    frame_offset += ENCODER_OUTPUT_T;

    return ret;
}

// ********************** class ZipFormer  ****************************************
ZipFormer::ZipFormer(const std::string &encoder_path, const std::string &decoder_path, const std::string &joiner_path) {
    // memset(m_Rknn_app_ctx, 0, sizeof(rknn_zipformer_context_t));
    // memset(m_Vocab, 0, sizeof(m_Vocab));
    // memset(m_Audio, 0, sizeof(audio_buffer_t));

    m_Rknn_app_ctx = std::make_shared<rknn_zipformer_context_t>();
    m_Audio = std::make_shared<audio_buffer_t>();
    memset(m_Vocab, 0, sizeof(m_Vocab));

    int ret;
    ret = read_vocab(VOCAB_PATH, m_Vocab);
    if (ret != 0) {
        spdlog::error("read vocab fail! ret={} vocab_path={}\n", ret, VOCAB_PATH);
        spdlog::info("exited!");
        exit(0);
    }

    m_Timer.tik();
    ret = this->init_zipformer_model(encoder_path, &m_Rknn_app_ctx->encoder_context);
    if (ret != 0) {
        spdlog::error("init_zipformer_model fail! ret={} encoder_path={}\n", ret, encoder_path);
        spdlog::info("exited!");
        exit(0);
    }
    this->build_input_output(&m_Rknn_app_ctx->encoder_context);
    m_Timer.tok();
    // m_Timer.print_time("init_zipformer_encoder_model");
    spdlog::info("-- init_zipformer_encoder_model use: {:.2f} ms", m_Timer.get_time());

    m_Timer.tik();
    ret = this->init_zipformer_model(decoder_path, &m_Rknn_app_ctx->decoder_context);
    if (ret != 0) {
        spdlog::error("init_zipformer_model fail! ret={} decoder_path={}\n", ret, decoder_path);
        spdlog::info("exited!");
        exit(0);
    }
    this->build_input_output(&m_Rknn_app_ctx->decoder_context);
    m_Timer.tok();
    // m_Timer.print_time("init_zipformer_decoder_model");
    spdlog::info("-- init_zipformer_decoder_model use: {:.2f} ms", m_Timer.get_time());

    m_Timer.tik();
    ret = this->init_zipformer_model(joiner_path, &m_Rknn_app_ctx->joiner_context);
    if (ret != 0) {
        spdlog::error("init_zipformer_model fail! ret={} oiner_path={}\n", ret, joiner_path);
        spdlog::info("exited!");
        exit(0);
    }
    this->build_input_output(&m_Rknn_app_ctx->joiner_context);
    m_Timer.tok();
    // m_Timer.print_time("init_zipformer_joiner_model");
    spdlog::info("-- init_zipformer_joiner_model use: {:.2f} ms", m_Timer.get_time());
};

ZipFormer::~ZipFormer() {
    // if (m_Audio->data) {
    //     free(m_Audio->data);
    // }

    for (int i = 0; i < VOCAB_NUM; ++i) {
        if (m_Vocab[i].token) {
            free(m_Vocab[i].token);
            m_Vocab[i].token = nullptr;
        }
    }

    int ret = 0;
    ret = this->release_zipformer_model(&m_Rknn_app_ctx->encoder_context);
    if (ret != 0) {
        spdlog::error("release_zipformer_model encoder_context fail! ret={}\n", ret);
    }

    ret = this->release_zipformer_model(&m_Rknn_app_ctx->decoder_context);
    if (ret != 0) {
        spdlog::error("release_zipformer_model decoder_context fail! ret={}\n", ret);
    }

    ret = this->release_zipformer_model(&m_Rknn_app_ctx->joiner_context);
    if (ret != 0) {
        spdlog::error("release_zipformer_model joiner_context fail! ret={}\n", ret);
    }
    spdlog::info("模型释放, ret={}\n", ret);
}

int ZipFormer::init_zipformer_model(const std::string &model_path, rknn_app_context_t *app_ctx) {
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    ret = rknn_init(&ctx, (void *)model_path.c_str(), model_len, 0, NULL);
    if (ret < 0) {
        spdlog::error("rknn_init fail! ret={}\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        spdlog::error("rknn_query fail! ret={}\n", ret);
        return -1;
    }
    spdlog::debug("model input num: {}, output num: {}\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    spdlog::debug("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            spdlog::error("rknn_query fail! ret={}\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    spdlog::debug("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            spdlog::error("rknn_query fail! ret={}\n", ret);
            return -1;
        }
        // dump_tensor_attr(&(output_attrs[i]));
    }

    // Set to context
    app_ctx->rknn_ctx = ctx;
    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    return 0;
};

void ZipFormer::build_input_output(rknn_app_context_t *app_ctx) {
    app_ctx->inputs = (rknn_input *)malloc(app_ctx->io_num.n_input * sizeof(rknn_input));
    memset(app_ctx->inputs, 0, app_ctx->io_num.n_input * sizeof(rknn_input));

    for (int i = 0; i < app_ctx->io_num.n_input; i++) {
        app_ctx->inputs[i].index = i;

        if (app_ctx->input_attrs[i].type == RKNN_TENSOR_FLOAT16) {
            app_ctx->inputs[i].size = app_ctx->input_attrs[i].n_elems * sizeof(float);
            app_ctx->inputs[i].type = RKNN_TENSOR_FLOAT32;
            app_ctx->inputs[i].fmt = app_ctx->input_attrs[i].fmt;
            app_ctx->inputs[i].buf = (float *)malloc(app_ctx->inputs[i].size);
            memset(app_ctx->inputs[i].buf, 0, app_ctx->inputs[i].size);
        }
        else if (app_ctx->input_attrs[i].type == RKNN_TENSOR_INT64) {
            app_ctx->inputs[i].size =
                app_ctx->input_attrs[i].n_elems * sizeof(int64_t);
            app_ctx->inputs[i].type = RKNN_TENSOR_INT64;
            app_ctx->inputs[i].fmt = app_ctx->input_attrs[i].fmt;
            app_ctx->inputs[i].buf = (int64_t *)malloc(app_ctx->inputs[i].size);
            memset(app_ctx->inputs[i].buf, 0, app_ctx->inputs[i].size);
        }
    }

    app_ctx->outputs = (rknn_output *)malloc(app_ctx->io_num.n_output * sizeof(rknn_output));
    memset(app_ctx->outputs, 0, app_ctx->io_num.n_output * sizeof(rknn_output));

    for (int i = 0; i < app_ctx->io_num.n_output; i++) {
        app_ctx->outputs[i].index = i;

        if (app_ctx->output_attrs[i].type == RKNN_TENSOR_FLOAT16) {
            app_ctx->outputs[i].size =
                app_ctx->output_attrs[i].n_elems * sizeof(float);
            app_ctx->outputs[i].is_prealloc = true;
            app_ctx->outputs[i].want_float = 1;
            app_ctx->outputs[i].buf = (float *)malloc(app_ctx->outputs[i].size);
            memset(app_ctx->outputs[i].buf, 0, app_ctx->outputs[i].size);
        }
        else if (app_ctx->output_attrs[i].type == RKNN_TENSOR_INT64) {
            app_ctx->outputs[i].size =
                app_ctx->output_attrs[i].n_elems * sizeof(int64_t);
            app_ctx->outputs[i].is_prealloc = true;
            app_ctx->outputs[i].want_float = 0;
            app_ctx->outputs[i].buf = (int64_t *)malloc(app_ctx->outputs[i].size);
            memset(app_ctx->outputs[i].buf, 0, app_ctx->outputs[i].size);
        }
    }
}

int ZipFormer::inference_zipformer_model(std::vector<std::string> &recognized_text, std::vector<float> &timestamp, float &audio_length) {
    int ret;
    recognized_text.clear();
    timestamp.clear();

    float *encoder_input = (float *)m_Rknn_app_ctx->encoder_context.inputs[0].buf;
    float *encoder_output = (float *)m_Rknn_app_ctx->encoder_context.outputs[0].buf;
    int64_t *hyp = (int64_t *)m_Rknn_app_ctx->decoder_context.inputs[0].buf;
    float *decoder_output = (float *)m_Rknn_app_ctx->decoder_context.outputs[0].buf;
    float *joiner_output = (float *)m_Rknn_app_ctx->joiner_context.outputs[0].buf;

    knf::FbankOptions fbank_opts;
    fbank_opts.frame_opts.samp_freq = 16000;
    fbank_opts.mel_opts.num_bins = 80;
    fbank_opts.mel_opts.high_freq = -400;
    fbank_opts.frame_opts.dither = 0;
    fbank_opts.frame_opts.snip_edges = false;
    knf::OnlineFbank fbank(fbank_opts);

    int num_frames = 0;
    int num_processed_frames = 0;
    int offset = N_OFFSET;
    int segment = N_SEGMENT;
    float tail_pad_length = 0.0; // sec
    fbank.AcceptWaveform(SAMPLE_RATE, m_Audio->data, m_Audio->num_frames);
    num_frames = fbank.NumFramesReady();
    int frame_offset = 0;

    while ((num_frames - num_processed_frames) > 0) {
        if ((num_frames - num_processed_frames) < segment) {
            tail_pad_length = (segment - (num_frames - num_processed_frames)) / 100.0f; // sec
            std::vector<float> tail_paddings(int(tail_pad_length * SAMPLE_RATE));
            fbank.AcceptWaveform(SAMPLE_RATE, tail_paddings.data(), tail_paddings.size());
            fbank.InputFinished();
        }
        ret = get_kbank_frames(&fbank, num_processed_frames, segment, encoder_input);
        if (ret < 0) {
            break;
        }

        ret = greedy_search(m_Rknn_app_ctx.get(), encoder_output, decoder_output, hyp, joiner_output, m_Vocab, recognized_text, timestamp, num_processed_frames, frame_offset);
        if (ret < 0) {
            spdlog::error("greedy_search fail! ret={}\n", ret);
            goto out;
        }
        num_processed_frames += offset;
    }

    audio_length = (float)m_Audio->num_frames / m_Audio->sample_rate + tail_pad_length;

out:

    return ret;
}

int ZipFormer::release_zipformer_model(rknn_app_context_t *app_ctx) {
    if (app_ctx->input_attrs != NULL) {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }

    if (app_ctx->output_attrs != NULL) {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }

    if (app_ctx->rknn_ctx != 0) {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }

    if (app_ctx != NULL) {
        release_input_output(app_ctx);
        app_ctx = NULL;
    }

    return 0;
}

std::string ZipFormer::run(const std::string &audio_path) {

    int ret;
    float infer_time = 0.0;
    float audio_length = 0.0;
    float rtf = 0.0;
    int frame_shift_ms = 10;
    int subsampling_factor = 4;
    float frame_shift_s = frame_shift_ms / 1000.0 * subsampling_factor;
    std::vector<std::string> recognized_text;
    std::vector<float> timestamp;

    std::string result = "";

    do {
        m_Timer.tik();
        ret = read_audio(audio_path.c_str(), m_Audio.get());
        if (ret != 0) {
            spdlog::error("read audio fail! ret={} audio_path={}\n", ret, audio_path);
            break;
        }

        if (m_Audio->num_channels == 2) {
            spdlog::info("convert_channels: {} -> {}", m_Audio->num_channels, 1);
            ret = convert_channels(m_Audio.get());
            if (ret != 0) {
                spdlog::error("convert channels fail! ret={}\n", ret, audio_path);
                break;
            }
        }

        if (m_Audio->sample_rate != SAMPLE_RATE) {
            spdlog::info("resample_audio: {} HZ -> {} HZ", m_Audio->sample_rate, SAMPLE_RATE);
            ret = resample_audio(m_Audio.get(), m_Audio->sample_rate, SAMPLE_RATE);
            if (ret != 0) {
                spdlog::error("resample audio fail! ret={}\n", ret, audio_path);
                break;
            }
        }
        m_Timer.tok();
        // m_Timer.print_time("read_audio & convert_channels & resample_audio");
        spdlog::info("-- read_audio & convert_channels & resample_audio use: {} ms", m_Timer.get_time());

        m_Timer.tik();
        ret = this->inference_zipformer_model(recognized_text, timestamp, audio_length);
        if (ret != 0) {
            spdlog::error("inference_zipformer_model fail! ret={}\n", ret);
            break;
        }
        m_Timer.tok();
        // m_Timer.print_time("inference_zipformer_model");
        spdlog::info("zipformer_inference use: {} ms", m_Timer.get_time());

        infer_time = m_Timer.get_time() / 1000.0; // sec
        rtf = infer_time / audio_length;
        // printf("\nReal Time Factor (RTF): %.3f / %.3f = %.3f\n", infer_time, audio_length, rtf);
        spdlog::info("Real Time Factor (RTF): {} / {} = {}", infer_time, audio_length, rtf);

        /* // print result
        std::cout << "\nTimestamp (s): ";
        std::cout << std::fixed << std::setprecision(2);
        for (size_t i = 0; i < timestamp.size(); ++i) {
            std::cout << timestamp[i] * frame_shift_s;
            if (i < timestamp.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        */

        result = std::accumulate(recognized_text.cbegin(), recognized_text.cend(), std::string(""));
    } while (0);

    return result;
}

} // namespace ZipFormer
