#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "log.hpp"

extern logging::logger< logging::file_log_policy > log_inst;


#ifdef LOGGING_LEVEL_1
#define LOG( id ) log_inst.print< logging::severity_type::debug , id >
#define LOG_ERR( id ) log_inst.print< logging::severity_type::error , id >
#define LOG_WARN( id ) log_inst.print< logging::severity_type::warning , id >
#else
#define LOG(...) 
#define LOG_ERR(...)
#define LOG_WARN(...)
#endif

#ifdef LOGGING_LEVEL_2
#define ELOG log_inst.print< logging::severity_type::debug >
#define ELOG_ERR log_inst.print< logging::severity_type::error >
#define ELOG_WARN log_inst.print< logging::severity_type::warning >
#else
#define ELOG(...) 
#define ELOG_ERR(...)
#define ELOG_WARN(...)
#endif

#endif
