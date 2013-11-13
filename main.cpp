#define LOGGING_LEVEL_1
#define LOGGING_LEVEL_2

#include "logger.hpp"
#include <thread>
#include <chrono>
#include <fstream>
#include <string>
#include <utility>
#include <random>
#include <cassert>
using namespace std;

const short num_of_prints{ 5 };

string prints[] = {
	"la merda vola e i soldi parlano",
	"sopra la panca la capra campa sotto la panca la capra crepa",
	"carpe diem",
	"bello il tempo oggi e'? Spero che non piova nel pomeriggio",
	"non e' bello cio' che e' bello ma e' bello cio' che piace"
};

void heavily_logging_thread( long amount )
{
	log_inst.set_thread_name( "THREAD" );
	auto gen = bind( uniform_int_distribution<>{ 0 , 4 }, default_random_engine{} );
	for( long i = 0 ; i < amount ; i++ )
	{
		LOG(0)("heavily_logging_thread(): This is one (",i,") of ", amount, " prints, thread: ",this_thread::get_id(),", ", prints[ gen() ] );
	}
}

int main()
{
	log_inst.set_thread_name( "MAIN" );
	fstream io_file("exec_info.txt" , ios_base::in);
	long num = 0;
	long last_tot_execution_time = 0;
	long last_print_execution_time = 0;
	if( io_file )
	{
		//Get last line
		string buffer, buffer_t;
		while( getline( io_file , buffer_t ) )
		{
			if( buffer_t.size() )
				buffer = buffer_t;
		}	
		//Last line!
		size_t pos = buffer.find_first_of(':');
		if( pos != std::string::npos )
		{
			//Get the execution number
			num = atoi( buffer.substr( 0 , pos ).c_str() );
		}
		//Now get the average execution time to calculat the difference
		long* cur = &last_tot_execution_time;
		short cnt = 2;
		do{
			pos = buffer.find_first_of( ':' , pos + 1 );
			if( pos != std::string::npos )
			{
				//Ok, we got something.
				pos += 2;
				size_t end = buffer.find_first_of( ' ' , pos );
				*cur = atoi( buffer.substr( pos, end - pos ).c_str() );
				assert( *cur != 0 );
				cur = &last_print_execution_time;
			}
			else
			{
				assert( 0 );
			}
		}while( --cnt );
		assert( last_tot_execution_time && last_print_execution_time );
	}
	cout<<"Amount of available data line: "<<num<<endl;

	io_file.close();
	io_file.open("exec_info.txt" , ios_base::out | ios_base::app );

	if( io_file )
	{
		long amount = 20000;
		short exec_loop = 5;

		chrono::nanoseconds duration{};
		for( short i = 0 ; i < exec_loop ; i++ )
		{
			auto begin = chrono::high_resolution_clock::now();

			thread th1{ heavily_logging_thread, amount };
			thread th2{ heavily_logging_thread, amount };
			thread th3{ heavily_logging_thread, amount };

			for( long i = 0 ; i < amount ; i++ )
			{
				LOG(0)("main(): This is one (",i,") of ", amount, " prints that are going to be printed by the thread ",this_thread::get_id() );
			}

			th1.join();
			th2.join();
			th3.join();
			auto end = chrono::high_resolution_clock::now();
			duration += chrono::duration_cast< chrono::nanoseconds >( end - begin );
		}
		chrono::nanoseconds per_print{ duration / ( amount * exec_loop * 4 ) };
		duration /= exec_loop;
		++num;
		long avg_exec_time = chrono::duration_cast< chrono::microseconds >( duration ).count();
		long avg_per_print = per_print.count();
		io_file<< num <<": Avg execution time: "<< avg_exec_time <<" us ("<<( (double) ( avg_exec_time - last_tot_execution_time ) / last_tot_execution_time ) * 100<<"%) \tAvg per print: "<< avg_per_print <<" ns (" << ( (double) ( avg_per_print - last_print_execution_time ) / last_print_execution_time ) * 100<<"%) \tExec. loop:"<<exec_loop<<"\tThreads: "<<4<<"\tPrints: "<< amount*4*exec_loop<<endl;
	}
	return 0;
}
