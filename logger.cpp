#include "logger.hpp"

logging::logger< logging::file_log_policy > log_inst( "execution.log" ,
                                                      logging::severity_type::debug1);
