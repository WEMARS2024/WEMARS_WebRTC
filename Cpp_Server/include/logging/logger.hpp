#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <rtc/rtc.hpp>

// create a custom namespace for our logger and give it an init function as an attribute

// logger namespace is used by the webRTC application to applied formatted logging easily
// for more information on spdlog, check https://github.com/gabime/spdlog.
namespace Logger {
    // default log level is INFO, can be changed by user
    // available settings (by severity): [Trace, Debug, Info, Warn, Error, Critical] (can also be set Off)
    // init expects two parameters allowing the user to set log level for general & rtc logger independently
    // they have default levels (INFO) set
    inline void init(spdlog::level::level_enum log_level = spdlog::level::info, rtc::LogLevel rtc_log_level = rtc::LogLevel::Info){

        // set the loggers pattern [year-month-day, hours-minutes-seconds] [thread id] [class name]
        // class name is set by using `spdlog::stdout_color_mt("class name")
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Thread:%t] [%n] [%^%l%$] %v");
        spdlog::set_level(log_level);

        // set rtclogger name
        auto rtcLogger = spdlog::stdout_color_mt("LibDataChannel");

        // pipe rtc's logs to the rtcLogger
        rtc::InitLogger(rtc_log_level, [rtcLogger](rtc::LogLevel level, std::string msg) {
            switch (level) {
                case rtc::LogLevel::Error:   rtcLogger->error(msg); break;
                case rtc::LogLevel::Warning: rtcLogger->warn(msg);  break;
                case rtc::LogLevel::Info:    rtcLogger->info(msg);  break;
                case rtc::LogLevel::Debug:   rtcLogger->debug(msg); break;
                default: break;
            }
        });
    }

    // createLogger function checks if logger already exists and returns it if so
    // otherwise it will create a new logger with the provided name
    inline std::shared_ptr<spdlog::logger> createLogger(std::string logger_name){
        
        auto logger = spdlog::get(logger_name);

        if (!logger) {
            
            try{
                logger = spdlog::stdout_color_mt(logger_name);
            } catch (std::exception& e) {
                std::cerr << "Logger creation for {" << logger_name << "} failed:" << e.what() << std::endl;
            }

        };

        return logger;

    }
}