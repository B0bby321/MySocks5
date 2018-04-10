#include <iostream>
#include "connectionPool.h"
#include "socksCore.h"
#include <string>
#include <memory>
#include "entryPoint.h"
#include <vector>
#include <algorithm>
#include <sstream>
#include <type_traits>
using namespace std;





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
}
