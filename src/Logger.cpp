#include "Logger.h"
#include <stdio.h>
#include <stdexcept>
#include <mutex>

const char* mylog::msgTypeWords[2] = {"General", "Error"};


namespace mylog{
	std::unique_ptr<Logger<fileEndPoint>> globalLog;


	void setupGlobalLog(std::string filename){
		globalLog = std::move( std::unique_ptr<Logger<fileEndPoint>>(new Logger<fileEndPoint>(std::move(std::unique_ptr<fileEndPoint>(new fileEndPoint(filename))))) );
	}

	fileEndPoint::fileEndPoint(std::string filename){
		f = fopen( filename.c_str(), "a" );
		if(f == nullptr){
			throw std::runtime_error("Could not open file");
		}
	}
	fileEndPoint::~fileEndPoint(){
		if(f != nullptr){
			fclose(f);
		}
	}
	void fileEndPoint::operator()(std::string msg){
		std::mutex m;
		std::lock_guard<std::mutex> lock(m);
		if(fputs( msg.c_str() , f) == 0){
			throw std::runtime_error("Could not write to file");
		}
		if(fputc('\n', f) == 0){
			throw std::runtime_error("Could not write to file");
		}
	}
	
}

