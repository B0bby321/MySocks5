#include "socksCore.h"
#include <string.h>
#include <string>
#include <chrono>
#include <thread>

socksCore::socksCore(){
};

socksCore::socksCore(const socksCore& that){
	info = nullptr;
}

void socksCore::operator()(std::shared_ptr< connectionPool<socksCore, socksCore::handlerInformation> > pool){
	info = nullptr;	
	while(1){
		while(!info){
			if(pool->shouldTerminate()){
					return;
			}
			info = pool->getTask();
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // REMOVE LATER
		}
		switch(info->getStage()){
			case socksCoreInfo::stage::NEW_CONNECTION: handleNewConnection();
			break;
			case socksCoreInfo::stage::LISTEN_REQUEST: handleListenRequest();
			break;
			case socksCoreInfo::stage::TUNNELING: handleTunneling();
			break;	
			default: info = nullptr;
			break;
		}
		if(info){ //check to see if there is other stuff on requests and if its empty then dont add task
			pool->addTask(std::move(info));
		}


	}
}

struct verIdentify{
	uint8_t ver;
	uint8_t nmethods;
	uint8_t methods[255];
};

struct verIdentify_resp{
	uint8_t ver;
	uint8_t method;
};

void socksCore::handleNewConnection(){
	if(!info->getConnection().pollRecv()){
		return;
	}
	puts("Connection");
	struct verIdentify client;
	int got = info->getConnection().recv(&client, sizeof(client));
	if(got <= 0){
		info = nullptr;
		return;
	}
	verIdentify_resp res;
	res.ver = 0x5;
	res.method = 0x0;
	info->getConnection().send(&res, sizeof(res));
	info->setStage(socksCoreInfo::stage::LISTEN_REQUEST);
}

struct sockRequestFirstPart{
	uint8_t ver;
	uint8_t cmd;
	uint8_t zero;
	uint8_t dstType;
};

struct sockRequestIpv4{
	uint32_t ipAddress;
	uint16_t portNetworkOrder;
};

struct sockRequestIpv6{
	uint8_t ipAddress[16];
	uint16_t portNetworkOrder;
};

struct sockResponseIPv4{
	uint8_t ver;
	uint8_t response;
	uint8_t zero;
	uint8_t dstType;
	uint32_t ipAddress;
	uint16_t portNetworkOrder;
	sockResponseIPv4(uint8_t res=0x01, uint32_t ip=0, uint16_t portNO=0): ver(0x5), 
	response(res), zero(0), dstType(0x01), 
	ipAddress(ip), portNetworkOrder(portNO){}
};

void socksCore::handleListenRequest(){
	//puts("Handling request");
	if(!info->getConnection().pollRecv()){
		return;
	}
	char buffer[sizeof(sockRequestFirstPart) + 256 + sizeof(uint16_t) + 10];
    int got = info->getConnection().recv(buffer, sizeof(buffer));
	if(got <= sizeof(sockRequestFirstPart)){
		info = nullptr;
		return;
	}
	sockRequestFirstPart* fp = (sockRequestFirstPart*)(buffer);
	
	switch(fp->cmd){
		case 0x01: CommandConnect(buffer, got);
		break;
		default: {
				sockResponseIPv4 e(0x07); 
				info->getConnection().send(&e, sizeof(e));
		}break;
	}
	if(info){
		info->setStage(socksCoreInfo::stage::TUNNELING);
	}
}
#include <iostream> //REMOVE
void socksCore::CommandConnect(const char* buffer, size_t size){ //precondition is that the size is greater than sizeof(sockRequestFirstPart)
	const sockRequestFirstPart* fp = (const sockRequestFirstPart*)(buffer);
	if(fp->dstType == 0x01){
		puts("IP request");
		if((size < sizeof(sockRequestFirstPart) + sizeof(uint32_t) + sizeof(uint16_t))){
			puts("command fail");
			std::cout<<(sizeof(sockRequestFirstPart) + sizeof(sockRequestIpv4))<<"  "<<sizeof(sockRequestFirstPart) <<"  "<<sizeof(sockRequestIpv4)<<"  "<<size<<"\n";//REMOVE
			sockResponseIPv4 e(0x44); // REMOVE 0x44
			info->getConnection().send(&e, sizeof(e));
			info = nullptr;
			return;
		}
		puts("Trying to get the ip");
		const sockRequestIpv4* ipinfo = (const sockRequestIpv4*)(buffer + sizeof(sockRequestFirstPart));
		puts("got IP");
		struct sockaddr_in x;
		x.sin_family = AF_INET;
		x.sin_port = ipinfo->portNetworkOrder;
		x.sin_addr.s_addr = ipinfo->ipAddress;
		puts("Trying to connect");
		info->setDestinationConnection(entryPoint::connection(*((struct sockaddr*)&x)));
	}else if(fp->dstType == 0x03){
		uint8_t nameSize = (uint8_t)buffer[sizeof(sockRequestFirstPart)]; //first octet of buffer is the domain length
		if((sizeof(sockRequestFirstPart)+ 1 + nameSize + sizeof(uint16_t)) > size){ 
			sockResponseIPv4 e;
			info->getConnection().send(&e, sizeof(e));
			info = nullptr;
			return;
		}
		char* name = new char[nameSize+1];
		const char* nameloc = buffer + sizeof(sockRequestFirstPart) + 1;
		for(int i =0; i<nameSize; i++){
			name[i] = nameloc[i];
		}
		name[nameSize] = '\0';
		
		struct addrinfo hints, *res;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;  // use IPv4
		hints.ai_socktype = SOCK_STREAM;
		const uint16_t* portLoc = (const uint16_t*)(buffer + sizeof(sockRequestFirstPart) + 1 + nameSize);
		if(getaddrinfo(name, nullptr, &hints, &res) != 0){
			sockResponseIPv4 e(0x04);
			info->getConnection().send(&e, sizeof(e));
			info = nullptr;
			return;
		}
		((struct sockaddr_in*)(res->ai_addr))->sin_port = *portLoc;
		info->setDestinationConnection(entryPoint::connection(*(res->ai_addr)));
		freeaddrinfo(res);
	}/*else if(fp->dstType == 0x04){
		puts("IP request");
		if((sizeof(sockRequestFirstPart) + sizeof(sockRequestIpv6)) > size){
			puts("command fail");
			sockResponseIPv4 e;
			info->getConnection().send(&e, sizeof(e));
			info = nullptr;
			return;
		}
		puts("Trying to get the ip");
		const sockRequestIpv4* ipinfo = (const sockRequestIpv4*)(buffer + sizeof(sockRequestFirstPart));
		puts("got IP");
		struct sockaddr_in x;
		x.sin_family = AF_INET;
		x.sin_port = ipinfo->portNetworkOrder;
		x.sin_addr.s_addr = ipinfo->ipAddress;
		puts("Trying to connect");
		info->setDestinationConnection(entryPoint::connection(*((struct sockaddr*)&x)));
		puts("Made Connection");
	}*/else{
		sockResponseIPv4 e(0x08);
		info->getConnection().send(&e, sizeof(e));
		info = nullptr;
		return;
	}

	if(info->getDestinationConnection().isEmpty()){
		std::cout<<"Failed to connect to "<< info->getDestinationConnection().getClientIP() <<"\n";
		sockResponseIPv4 e(0x05);
		info->getConnection().send(&e, sizeof(e));
		info = nullptr;
		return;
	}
	auto respIPStor = info->getDestinationConnection().getAddress();
	struct sockaddr_in respIP = *((struct sockaddr_in*)(&respIPStor));
	sockResponseIPv4 res(0, respIP.sin_addr.s_addr, respIP.sin_port);
	info->getConnection().send(&res, sizeof(res));
	puts("Made Connection");

}


void socksCore::handleTunneling(){
	//puts("Tunneling");
	char buffer[1000000];
	if(info->getConnection().pollRecv()){
		int got = info->getConnection().recv(buffer, sizeof(buffer));
		if(got < 0){
			puts("Ended session becauses of fail to recv from host");
			info = nullptr;
			return;
		}
		
		got = info->getDestinationConnection().send(buffer, got);
		if(got<0){
			puts("Ended session becauses of fail to send from host to server");
			info = nullptr;
			return;
		}
		puts("CON-->DEST");
	}
	if(info->getDestinationConnection().pollRecv()){
		//memset(buffer, 0, sizeof(buffer));
		int got = info->getDestinationConnection().recv(buffer, sizeof(buffer));
		if(got < 0){
			puts("Ended session becauses of fail to recv from server");
			info = nullptr;
			return;
		}
		int sent = info->getConnection().send(buffer, got);
		if(sent != got){
			puts("Ended session becauses of fail to send from server to host");
			info = nullptr;
			return;
		}
		return;
	}
	
}












socksCore::socksCoreInfo::socksCoreInfo(entryPoint::connection&& con):c(std::move(con)), s(socksCore::socksCoreInfo::stage::NEW_CONNECTION){
}

socksCore::socksCoreInfo::stage socksCore::socksCoreInfo::getStage(){
	return s;
}
void socksCore::socksCoreInfo::setStage(stage st){
	s = st;
}
const entryPoint::connection& socksCore::socksCoreInfo::getConnection(){
	return c;
}
const entryPoint::connection& socksCore::socksCoreInfo::getDestinationConnection(){
	return dest;
}
void socksCore::socksCoreInfo::setDestinationConnection(entryPoint::connection&& con){
	dest = std::move(con);
}





