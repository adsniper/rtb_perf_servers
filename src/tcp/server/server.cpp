// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*
 * Copyright (c) 2019, AdSniper, Oleg Romanenko (oleg@romanenko.ro)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <csignal>
#include <sniper/event/Loop.h>
#include <sniper/event/Sig.h>
#include <sniper/event/Timer.h>
#include <sniper/http/Server.h>
#include <sniper/log/log.h>
#include <sniper/std/check.h>
#include <sniper/std/vector.h>
#include <sniper/threads/Stop.h>
#include <thread>
#include "Config.h"

using namespace sniper;

void stop_signal(const event::loop_ptr& loop)
{
    log_warn("Stop signal. Exiting");
    threads::Stop::get().stop();
    loop->break_loop(ev::ALL);
}

static void sigsegv_handler(int sig)
{
    log_err("Error: signal {} ({})", strsignal(sig), sig);
    log_err("{}", stacktrace_to_string(boost::stacktrace::stacktrace()));
    exit(1);
}

class Node final
{
public:
    explicit Node(const tcp_test::server::Config& config) : _config(config)
    {
        for (unsigned i = 0; i < _config.threads_count(); i++) {
            _t.emplace_back([this] {
                try {
                    worker();
                }
                catch (std::exception& e) {
                    log_err(e.what());
                }
                catch (...) {
                    log_err("[Node] non std::exception occured");
                }
            });
        }
    }

    ~Node()
    {
        for (auto& w : _t)
            if (w.joinable())
                w.join();
    }

private:
    void worker()
    {
        auto loop = event::make_loop();

        http::Server http_server(loop, _config.http_server_config());
        check(http_server.bind(_config.ip(), _config.port()), "[Node] cannot bind to {}:{}", _config.ip(),
              _config.port());

        string data;
        if (_config.response_size())
            data.resize(_config.response_size());

        http_server.set_cb_request([&](const auto& conn, const auto& req, const auto& resp) {
            if (!data.empty())
                resp->set_data_nocopy(data);

            resp->code = http::ResponseStatus::OK;
            conn->send(resp);
        });

        event::TimerRepeat timer_stop(loop, 1s, [&loop] {
            if (threads::Stop::get().is_stopped())
                loop->break_loop(ev::ALL);
        });

        loop->run();
    }

    const tcp_test::server::Config& _config;
    vector<std::thread> _t;
};

int main(int argc, char** argv)
{
    if (argc != 2) {
        log_info("{}, build {}, {} UTC, rev {}", APP_NAME, __DATE__, __TIME__, GIT_SHA1);
        log_err("Run: {} config.conf", APP_NAME);
        return EXIT_FAILURE;
    }

    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGSEGV, sigsegv_handler);
    std::signal(SIGABRT, sigsegv_handler);

    try {
        log_info("{} init", APP_NAME);

        auto loop = event::make_loop();
        if (!loop) {
            log_err("Main: cannot init event loop");
            return EXIT_FAILURE;
        }

        tcp_test::server::Config config(argv[1]);
        Node node(config);

        event::Sig sig_int(loop, SIGINT, stop_signal);
        event::Sig sig_iterm(loop, SIGTERM, stop_signal);

        log_info("{} started", APP_NAME);
        loop->run();
    }
    catch (std::exception& e) {
        log_err(e.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        log_err("Non std::exception occured");
        return EXIT_FAILURE;
    }

    log_info("{} stopped", APP_NAME);
    return EXIT_SUCCESS;
}
