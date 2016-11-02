#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <stdexcept>
#include <ctime>
#include <sstream>
#include <mutex>
#include <chrono>
#include <cstring>
#include <thread>
#include <map>
#include <queue>
#include <atomic>
#include <cassert>
#include "date.h"

#ifndef LOG_HPP
#define LOG_HPP

namespace logging
{

/*
         * log_policy for the logger
         */

class log_policy_interface
{
public:
    virtual void		open_out_stream(const std::string& name) = 0;
    virtual void		close_out_stream() = 0;
    virtual void		write(const std::string& msg) = 0;
    virtual ~log_policy_interface() = 0;
};

inline log_policy_interface::~log_policy_interface(){}

/*
         * Implementation which allow to write into a file
         */

class file_log_policy : public log_policy_interface
{
    std::ofstream out_stream;
public:
    file_log_policy(){   }
    void open_out_stream(const std::string& name);
    void close_out_stream();
    void write(const std::string& msg);
    ~file_log_policy(){}
};

enum class severity_type
{
    debug1 = 1,
    debug2,
    debug3,
    warning1,
    warning2,
    error      //6
};

/*
         * the Logger 		using namespace data;class, shall be instantiated with a specific log_policy
         */

template< typename log_policy >
class logger;

template< typename log_policy >
void logging_daemon( logger< log_policy >* logger )
{
    //Dump the log data if any
    std::unique_lock< std::timed_mutex > writing_lock{logger->write_mutex,std::defer_lock};
    do{
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        if( logger->log_buffer.size() )
        {
            writing_lock.lock();
            for( auto& elem : logger->log_buffer )
            {
                logger->policy.write( elem );
            }
            logger->log_buffer.clear();
            writing_lock.unlock();
        }
    }while( logger->is_still_running.test_and_set() || logger->log_buffer.size() );
    logger->policy.write( "-Terminating the logger daemon!-" );
}

template< typename log_policy >
class logger
{
    std::chrono::high_resolution_clock::time_point reference_epoch;

    unsigned log_line_number;
    severity_type logging_level;

    //static std::stringstream log_stream;
    log_policy policy;
    std::timed_mutex write_mutex;

    std::vector< std::string > log_buffer;
    std::thread daemon;
    std::atomic_flag is_still_running = ATOMIC_FLAG_INIT;

    //Core printing functionality
    void print_impl(std::stringstream&&);

    template<typename First, typename...Rest>
    void print_impl(std::stringstream&&,First&& parm1,Rest&&...parm);

    std::map<std::thread::id,std::string> thread_name;
public:
    logger(const std::string& name,severity_type min_log_severity);
    void set_logging_level(severity_type new_level);

    template< severity_type severity , typename...Args >
    void print(Args&&...args);

    void set_thread_name(const std::string& name);
    void terminate_logger();

    ~logger();

    template< typename policy >
    friend void logging_daemon( logger< policy >* logger );
};

/*
         * Implementation for logger
         */

template< typename log_policy >
void logger< log_policy >::terminate_logger()
{
    //Terminate the daemon activity
    is_still_running.clear();
    daemon.join();
}

template< typename log_policy >
void logger< log_policy >::set_thread_name( const std::string& name )
{
    thread_name[ std::this_thread::get_id() ] = name;
}

template< typename log_policy >
template< severity_type severity ,typename...Args >
void logger< log_policy >::print(Args&&...args)
{
    if(severity < logging_level){
        return;//Level too low
    }
    std::stringstream log_stream;
    //Prepare the header

    auto now = std::chrono::system_clock::now();

    {
        using namespace date;
        log_stream << log_line_number++ <<" "<< now <<" ";
    }

    log_stream << thread_name[ std::this_thread::get_id() ] <<" ";

    switch( severity )
    {
    case severity_type::debug1:
    case severity_type::debug2:
    case severity_type::debug3:
        log_stream<<"DBG: ";
        break;
    case severity_type::warning1:
    case severity_type::warning2:
        log_stream<<"WRN: ";
        break;
    case severity_type::error:
        log_stream<<"ERR: ";
        break;
    };

    print_impl(std::forward<std::stringstream>(log_stream),
               std::move(args)...);
}

template< typename log_policy >
void logger< log_policy >::print_impl(std::stringstream&& log_stream)
{
    std::lock_guard< std::timed_mutex > lock( write_mutex );
    log_buffer.push_back(log_stream.str());
    std::cout<<log_stream.str()<<std::endl<<std::flush;
}

template< typename log_policy >
template< typename First, typename...Rest >
void logger< log_policy >::print_impl(std::stringstream&& log_stream,
                                      First&& parm1,Rest&&...parm)
{
    log_stream << parm1;
    print_impl(std::forward<std::stringstream>(log_stream),
               std::move(parm)...);
}

template< typename log_policy >
logger< log_policy >::logger(const std::string& name,
                             severity_type min_log_severity):
    log_line_number{0},
    logging_level{min_log_severity}
{
    policy.open_out_stream( name );
    reference_epoch = std::chrono::high_resolution_clock::now();
    //Set the running flag ans spawn the daemon
    is_still_running.test_and_set();
    daemon = std::move( std::thread{ logging_daemon< log_policy > , this } );
}

template< typename log_policy >
void logger<log_policy>::set_logging_level(severity_type new_level)
{
    if(new_level > severity_type::error){
        new_level = severity_type::error;
    }
    logging_level = new_level;
}

template< typename log_policy >
logger< log_policy >::~logger()
{
    terminate_logger();
    policy.write( "-Logger activity terminated-" );
    policy.close_out_stream();
}
}

#endif
