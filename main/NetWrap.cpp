#include "NetWrap.h"

using namespace std;

NetWrap::NetWrap(const char *server_ip, unsigned server_port){
	serverAddress.sin_family = AF_INET;
	inet_pton(AF_INET, server_ip, &serverAddress.sin_addr.s_addr);
	serverAddress.sin_port = htons(server_port);

	
    socket_dsc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(socket_dsc < 0)
		printf("socket error. errno: %s\n",strerror(errno));
	
}

bool 
NetWrap::connect(){
	int rc = ::connect(socket_dsc, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));
	if(rc != 0){
		cout<<"connection failed. errno: "<<strerror(errno)<<endl;
		return false;
	} else 
		cout<<"connected to server."<<endl;
	
	return true;
}

bool
NetWrap::disconnect(){
	if(socket_dsc > 0)
		close(socket_dsc);

	socket_dsc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(socket_dsc < 0){
		printf("socket error. errno: %s\n",strerror(errno));
		return false;
	}
	return true;
}



bool 
NetWrap::send(Packet& p){
	char buf[512];
	p.serialize(buf);
	int w_check;
	int packet_size = p.get_packet_size();
	w_check = write(socket_dsc,&packet_size,sizeof(int));
	if(w_check <= 0){
		cout<<"write error. errno: "<<strerror(errno)<<endl;
		return false;
	}

	w_check = write(socket_dsc,buf,packet_size);
	if(w_check <= 0){
		cout<<"write error. errno: "<<strerror(errno)<<endl;
		return false;
	}
	return true;
}


NetWrap::~NetWrap(){
	if(socket_dsc > 0)
		close(socket_dsc);
	socket_dsc = -1;
};

bool NetWrap::wait_start(){
	int r_check;
	int a;
	r_check = read(socket_dsc, &a, sizeof(int));
	if(r_check <= 0){
		cout<<"read error. errno: "<<strerror(errno)<<endl;
		return false;
	}
	return true;
}