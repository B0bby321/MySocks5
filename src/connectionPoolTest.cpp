#include <iostream>
#include "connectionPool.h"
#include "socksCore.h"
#include <string>
#include <memory>
#include "Logger.h"
#include <chrono>
#include "entryPoint.h"
#include <vector>
#include <algorithm>
#include <sstream>
#include <type_traits>
using namespace std;
struct handi{
	string h;
	handi(string s):h(s){}
};


struct hand{
	void operator()(std::shared_ptr< connectionPool<hand, handi> > pool){
		while(1){
			auto t = pool->getTask();
			while(!t){
				if(pool->shouldTerminate()){
					return;
				}
				t = pool->getTask();
			}
			cout<<t->h<<endl;
		}
	}
};

template<typename C>
void sendToAll(string str, const C &vec, typename C::const_iterator sender){
	for(typename C::const_iterator begin = vec.begin(); begin != vec.end(); ++begin){
		if(begin == sender){
			continue;
		}
		begin->send(str.c_str(), str.size());
	}
}

int main(){
	entryPoint e(1080);
	socksCore handle;
	auto pool = connectionPool<socksCore, socksCore::handlerInformation>::newConnectionPool(1, handle);
	while(1){
		if(e.pollAccept()){
			pool->addTask(unique_ptr<socksCore::handlerInformation>(new socksCore::handlerInformation(e.accept())));
		}
	}

	return 0;
	/*
	entryPoint e(1080);
	vector<entryPoint::connection> c;
	while(1){
		if(e.pollAccept()){
			c.push_back(e.accept());
			if(c.back().isEmpty()){
				continue;
			}
			cout<< c.back().getClientIP() << endl;
		}
		for(auto i = c.begin(); i!=c.end(); ++i){
			char buffer[100];
			if(i->pollRecv()){
				int rec = i->recv(buffer, sizeof(buffer)-1);
				if(rec == 0){
					continue;
				}
				if(rec == -1){
					c.erase(i);
					i--;
					continue;
				}
				buffer[rec] = 0;
				string echo(buffer);
				echo.erase(remove(echo.begin(),echo.end(), '\n'), echo.end());
				stringstream ss;
				ss<<i-c.begin()<<": "<<echo<<'\n';
				sendToAll(ss.str(),c ,i);
			}
		}
		
		
	}


	return 0;

	/*
	log::setupGlobalLog("socks.log");
	if(log::globalLog){
		chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
		log::globalLog->log("Thread pool created");
		 chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
		cout<< chrono::duration_cast<chrono::microseconds>( t2 - t1 ).count() <<endl;
	}
	/*
	return 0;
	hand h;
	auto pool = connectionPool<hand, handi>::newConnectionPool(5, h);
	


	for(int i=0; i<50; i++){
		pool->addTask( unique_ptr<handi>(new handi("Hello")) );
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	pool->terminateAll();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	cout<<pool.use_count()<<endl;
	return 0;
	*/
}
