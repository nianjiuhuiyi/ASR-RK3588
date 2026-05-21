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

#include <fstream>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <httplib.h>

#include "zipformer.h"

static constexpr int port = 6789;

std::string encoder_path;
std::string decoder_path;
std::string joiner_path;

static std::atomic<bool> keepRuning(true);

static void signalHandler(int signum) {
    spdlog::info("Interrupt signal ({}) received.", signum);
    keepRuning.exchange(false);
}

/**
 * @brief 注册日志写入到文件中
 *
 * @param method_name [in] 识别方法的名称
 * @param flush_interval [in] 每多少秒往硬盘中写入一次日志
 */
void register_logger(const std::string &method_name, const int &flush_interval) {

    std::string loger_path = "logs/" + method_name + "/" + method_name + ".log";
    std::shared_ptr<spdlog::logger> file_logger = spdlog::rotating_logger_mt("file_log", loger_path, 1024 * 1024 * 100, 10);

    // 遇到warn flush日志，防止丢失
    file_logger->flush_on(spdlog::level::warn);
    //每三秒刷新一次（不是一直往硬盘中写，提高效率）
    spdlog::flush_every(std::chrono::seconds(flush_interval));

    spdlog::set_default_logger(file_logger);
    // Set global log level to debug
    // spdlog::set_level(spdlog::level::debug);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
}

/**
 * @brief 语音识别服务，接收POST请求，处理请求中form-data中key为"audio"的.wav格式的音频文件，并返回对应文本结果
 *
 * @param server httplib实例化对象的指针
 * @param zipformer zipformer实例化对象的指针
 */
// void run_server(httplib::Server *server, ZipFormer::ZipFormer *zipformer) {
void run_server(httplib::Server *server) {
    // 创建存音频文件的临时文件夹
    std::string file_save_path = "./temp/";
    try {
        if (!std::filesystem::exists(file_save_path)) {
            std::filesystem::create_directories(file_save_path);
        }
    }
    catch (const std::filesystem::filesystem_error &e) {
        spdlog::error("[ERROR] Error creating or checking folder: {}", e.what());
        return;
    }

    // 1、/hi get路由
    server->Get("/hi", [](const httplib::Request &, httplib::Response &res) { res.set_content("hello world!", "text/plain"); });

    // 2、/upload post路由，上传key为'audio'的格式为'.wav'的音频文件。
    server->Post("/upload", [&](const httplib::Request &req, httplib::Response &res) {
        std::string key = "audio";
        auto size = req.files.size();
        bool ret = req.has_file(key);
        if (!ret) {
            // 错误请求
            res.status = 400;
            res.set_content("文件key错误，请检查是否为: " + key, "text/plain");
            return;
        }

        const auto &file = req.get_file_value(key);
        std::string filename = file.filename;
        // 仅支持.wav 格式的数据
        if (filename.substr(filename.size() - 4) != ".wav") {
            res.status = 400;
            res.set_content("数据类型不支持，仅支持'.wav'格式的音频文件.", "text/plain");
            return;
        }

        std::string filepath = file_save_path + filename;
        std::ofstream ofs(filepath, std::ios::binary);
        if (!ofs.is_open()) {
            res.status = 500; // 服务器内部错误
            res.set_content("Failed to open file", "text/plain");
            return;
        }

        ofs.write(file.content.data(), file.content.size());
        ofs.close();

        // 检测结果
        try {
            // TODO: 每次实例化都大约要两百多毫秒，但这个zipformer识别到一定程序就是会乱码，一定要重新初始化对象
            // 音频识别对象(运行到一定程度，就会出现结果看起来是错的，所以每次运行就实例化一次对象)
            ZipFormer::ZipFormer zipformer(encoder_path, decoder_path, joiner_path);

            std::string result = zipformer.run(filepath);
            spdlog::info("{}: {}", filename, result);

            res.status = 200;
            res.set_content(result, "text/plain");
            std::filesystem::remove(filepath);
        }
        catch (...) {
            res.status = 500;
            res.set_content("语音识别服务内部错误.", "text/plain");
        }
    });

    spdlog::info("监听: http://192.168.108.149:6789\n");
    // 阻塞调用，直到对象调用 stop() 函数来安全结束。
    server->listen("0.0.0.0", port);
}

int main(int argc, char *argv[]) {
    encoder_path = argv[1];
    decoder_path = argv[2];
    joiner_path = argv[3];

    if (argc == 5) {
        std::string audio_path = argv[4];
        // 音频识别对象
        ZipFormer::ZipFormer zipformer(encoder_path, decoder_path, joiner_path);
        spdlog::info("result: {}", zipformer.run(audio_path));
    }
    else if (argc == 4) {
        // 注册信号处理程序
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // 输出日志到文件夹内 (注释掉就是输出到终端)
        register_logger("zipformer", 1);

        spdlog::info("\n************启动语音识别服务************");

        // 服务器对象
        httplib::Server server;
        // 服务线程
        std::thread server_thread(run_server, &server);

        while (keepRuning) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 停止服务
        server.stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }

        spdlog::info("服务已安全退出.");
    }
    else {
        spdlog::warn("参数输入错误，请参看README.\n");
    }

    return 0;
}
