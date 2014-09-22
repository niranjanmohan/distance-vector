/*
   Created by :Niranjan Mohan
   UB ID      :50097870
   Email ID   :nmohan4@buffalo.edu
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
using namespace std;
using namespace boost;

int no_serv=0;
//This is the structure to handle the node self details
struct  mynodeinfo{
	string n_sno;
	string n_portno;
	string n_ip;
	int sno;
	int edgeno;
	string n_edge[5];
	string n_edgeip[5];
	string n_edgeport[5];
	string n_cost[5];
};
//This is the structure to handle the Routing table and Node Table 
struct nodeinfo{
	string n_sno;
	string n_portno;
	string n_ip;
	string n_next;
	bool isEdge;
	bool isDisabled;
	int n_cost;
	int sno;
	int edgeno;
	int n_tval;
};

int main(int args,char *message[]){
	int opt,time_int,i;
	int sockfd,maxcon,bind_state;
	fd_set mainfdset,tempfdset;
	unsigned int packet_count=0;
	int no_recvbyts;
	sockfd=0;maxcon=0;bind_state=0;no_recvbyts=0;
	char buf[1000];
	char recvip[30];
	socklen_t addr_len;
	string choice,filename,msg,buff;
	vector<string> token;
	struct addrinfo remoteaddr;
	struct sockaddr_in peer,servaddr,serv;
	struct timeval tv;
	struct mynodeinfo myinfo;
	struct nodeinfo peerinfo[5];
	struct nodeinfo msginfo[5];
	struct nodeinfo routeinfo[5];
	//function Declaration
	void sendupdate(struct nodeinfo peerinfo[],struct mynodeinfo myinfo);
	void loadinfo(struct mynodeinfo &myinfo,struct nodeinfo peerinfo[],string filename);
	void display(struct nodeinfo routeinfo[],struct mynodeinfo myinfo );
	void loadmsg(struct nodeinfo msginfo[],struct nodeinfo peerinfo[],string msg);
	void updateDV(struct nodeinfo msginfo[],struct nodeinfo peerinfo[],struct nodeinfo routeinfo[],struct mynodeinfo myinfo);
	string getSNO(struct nodeinfo peerinfo[],string portno);
	bool updatePeerInfo(struct nodeinfo peerinfo[],struct nodeinfo routeinfo[],string edge_sno,string new_cost);
	void disableNode(struct nodeinfo peerinfo[],string d_sno);
	void checkPeerState(struct nodeinfo peerinfo[],struct nodeinfo msginfo[]);
	void updatePeerStates(struct nodeinfo peerinfo[],struct nodeinfo routeinfo[],struct nodeinfo msginfo[]);
	void *get_in_addr(struct sockaddr *sa);
	while((opt = getopt(args,message,":t:i:"))!= -1){
		switch(opt){
			case 't':
				filename = optarg;
				break;
			case 'i':
				time_int = atoi(optarg);
				break;
			default:
				exit(EXIT_FAILURE);
				break;
		}
	}
	//creating a socket
	loadinfo(myinfo,peerinfo,filename);
	copy(begin(peerinfo),end(peerinfo),begin(routeinfo));
	no_serv = myinfo.sno;
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset((char*)&serv,0,sizeof(struct sockaddr_in));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = INADDR_ANY;
	serv.sin_port = htons(atoi(myinfo.n_portno.c_str()));
	//finished setting the socket details 
	bind_state = bind(sockfd,(struct sockaddr*)&serv,sizeof (struct sockaddr_in));
	FD_ZERO(&mainfdset);
	FD_ZERO(&tempfdset);
	FD_SET(sockfd,&mainfdset);
	FD_SET(0,&mainfdset);
	//time val for the select function
	tv.tv_sec =time_int;
	tv.tv_usec=0;
	maxcon=sockfd;
	while(true)
	{
		tempfdset = mainfdset;
		select(sockfd+1,&tempfdset,NULL,NULL,&tv);
		if(FD_ISSET(3,&tempfdset)){
			memset(buf,0,sizeof(buf));
			no_recvbyts = recvfrom(sockfd,buf,1000,0,(struct sockaddr *)&peer,&addr_len);
			msg = buf;
			packet_count++;
			loadmsg(msginfo,peerinfo,msg);
			cout<<"Received Message from Server:"<<msginfo[0].n_sno<<"\n";
			//need to update the peer timer
			checkPeerState(peerinfo,msginfo);
			updateDV(msginfo,peerinfo,routeinfo,myinfo);
		//	display(routeinfo,myinfo);
		}
		else if(FD_ISSET(0,&tempfdset))
		{
			int c ;
			getline(cin,choice);
			to_lower(choice);
			split(token,choice,is_any_of(" "));
			if(token.size()==4 &&(token[0].compare("update")) ==0){
				//either update 					
				if(token[1].compare(myinfo.n_sno)==0){
					if(updatePeerInfo(peerinfo,routeinfo,token[2],token[3])){
						cout<<"Updated Successfully \n";
					}
				}
				else{
					cout<<"Invalid Node ID for this Server . This Server ID is:"<<myinfo.n_sno<<"\n";
				}
			}
			else if(token.size()==2 && token[0].compare("disable")==0){
				//code to disable
				disableNode(peerinfo,token[1]); 
				if(updatePeerInfo(peerinfo,routeinfo,token[1],"inf")){
					cout<<"Disabled Successfully \n";
				}
			}
			else if(choice.compare("step") == 0){
				sendupdate(peerinfo,myinfo);
				cout<<"STEP Success \n";
				FD_ZERO(&tempfdset);
			}
			else if(choice.compare("packets")==0){
				cout<<"The Number of Packet Received by Node is:"<<packet_count<<"\n";
				packet_count =0;
				tv.tv_sec = time_int;
			}	
			else if(choice.compare("display")==0){
				display(routeinfo,myinfo);
			}
			else if(choice.compare("crash")== 0){
				cout<<"Server Crashed\n";
				break;
			}
			else{
				cout<<"***************************HELP**************************\n";
				cout<<"Please Enter Valid Command And Parameters \n";
				cout<<"1 :For Update :UPDATE <Own Server:ID> <Server:ID2> <Link Cost> \n";
				cout<<"2 :STEP to send packets \n";
				cout<<"3 :DISABLE <Server:ID> to disable a link to the server \n";
				cout<<"4 :PACKETS to display the number of packets received \n";
				cout<<"5 :CRASH to crash the server .After crash EXIT will exit code \n";
				cout<<" *********************************************************\n";
			}
		}	
		else{
			//need to add time stamp and check if inactive then disable
			updatePeerStates(peerinfo,routeinfo,msginfo);
			sendupdate(peerinfo,myinfo);
			tv.tv_sec = time_int;
		}
	}
	while(true){
		getline(cin,choice);
		if(choice.compare("exit")== 0){
		cout<<"Exiting Program \n";
		exit(0);
		}
		cout<<"Server Crashed\n";
	}	
	return 0;
}


//*************************************myutil functions*************************************************


void  loadinfo(struct mynodeinfo &myinfo,struct nodeinfo peerinfo[5],string filename){
	int i,j ;
	i=0;
	j=0;
	fstream infile;
	string data2;
	string data;
	size_t found1,found2;
	string temp;
	infile.open(filename.c_str(),ios::in);
	string delim(" ");
	while(!infile.eof()){
		if(i <=1){
			//no of servers
			getline(infile,data);
			if(i==0){
				myinfo.sno = atoi(data.c_str());
			}			
			else{
				myinfo.edgeno =atoi(data.c_str());
			}
			//no of neighbours
		}
		else if(i >=2 && i <=myinfo.sno+1){
			//details 1
			getline(infile,data);
			found1 = data.find(delim);
			found2 = data.find(delim,found1+1);
			peerinfo[i-2].n_sno = data.substr(0,1);
			peerinfo[i-2].n_ip = data.substr(2,found2-2);
			peerinfo[i-2].n_portno = data.substr(found2+1);
		//	temp = peerinfo[i-2].n_portno.substr(0, peerinfo[i-2].n_portno.size()-1);
		//	peerinfo[i-2].n_portno = temp;
		}
		else{
			getline(infile,data);
			if(data.length() > 3){
				myinfo.n_sno=data.substr(0,1);
				myinfo.n_edge[j]= data.substr(2,1);
				myinfo.n_cost[j]= data.substr(3,4);
			//	cout<<"the cost loaded from file r table"<<myinfo.n_cost[j]<<"\n";
				j++;
			}
		}
		i++;
	}
	infile.close();
	//code to load my ip and port
	for(i=0;i<myinfo.sno;i++){
		//initializing default values to peerinfo
		peerinfo[i].isEdge = false;
		peerinfo[i].n_cost = 9999;
		peerinfo[i].isDisabled = false;
		peerinfo[i].n_tval=0;
		if(myinfo.n_sno.compare(peerinfo[i].n_sno)== 0){
			myinfo.n_ip = peerinfo[i].n_ip;
			myinfo.n_portno = peerinfo[i].n_portno; 
		}
		for(j=0;j<myinfo.sno;j++){
			if(myinfo.n_edge[j].compare(peerinfo[i].n_sno)== 0){
				myinfo.n_edgeip[j] = peerinfo[i].n_ip;
				myinfo.n_edgeport[j] = peerinfo[i].n_portno;
				peerinfo[i].isEdge = true;
				peerinfo[i].n_cost =atoi(myinfo.n_cost[j].c_str());
				peerinfo[i].n_next = peerinfo[i].n_sno;
			}
		}
		//coded for testing 
	//cout<<"ip is"<<peerinfo[i].n_ip<<"port#"<<peerinfo[i].n_portno<<"cost#"<<peerinfo[i].n_cost<<"Id#"<<peerinfo[i].n_sno<<"\n";
	}
}

void sendupdate(struct nodeinfo peerinfo[],struct mynodeinfo myinfo){
	// talker code here integrated
	int i;
	int csockfd;
	//struct addrinfo_in serv,peer;
	int no_byts;
	sockaddr_in peeraddr,servaddr;
	string buff;
	//int sockfd;
	fd_set mainfdset,tempfdset;
	char buf[1000];
	socklen_t addr_len;
	string sendbyts;
	int v=4;	//creating a socket
	csockfd = socket(AF_INET, SOCK_DGRAM, 0);
	//code to construct string to send information to peer
	sendbyts=lexical_cast<string>(v);
	sendbyts+=" ";
	sendbyts+=myinfo.n_portno;sendbyts+=" ";
	sendbyts+=myinfo.n_ip;sendbyts+=" ";
	sendbyts+="|";
	for(i=0;i<no_serv;i++){
		if(myinfo.n_sno.compare(peerinfo[i].n_sno)== 0){
			if(i != myinfo.sno){
				i++;
			}
		}
		if(i == myinfo.sno){
			break;
		}
		sendbyts+= peerinfo[i].n_ip;sendbyts.append(" ");
		sendbyts.append(peerinfo[i].n_portno);sendbyts.append(" ");
		sendbyts.append("0x0");
		sendbyts.append(" ");
		sendbyts.append(peerinfo[i].n_sno);sendbyts.append(" ");
		sendbyts+=lexical_cast<string>(peerinfo[i].n_cost);
		sendbyts.append("|");
	}
	for(i=0;i<no_serv;i++){
		if(peerinfo[i].isEdge && !(peerinfo[i].isDisabled)){
			memset((char*)&servaddr,0,sizeof(struct sockaddr_in));
			servaddr.sin_family = AF_INET;
			inet_aton(peerinfo[i].n_ip.c_str(),&servaddr.sin_addr);
			servaddr.sin_port = htons(atoi(peerinfo[i].n_portno.c_str()));
		//	cout<<"going to send update \n"<<sendbyts.c_str();
			no_byts=sendto(csockfd,sendbyts.c_str(),strlen(sendbyts.c_str()),0,(struct sockaddr*)&servaddr,sizeof(struct sockaddr_in));
		}
	}
	close(csockfd);
}

void display(struct nodeinfo routeinfo[],struct mynodeinfo myinfo) {
	int i;
	cout<<"\n<<-------------ROUTING TABLE --------------------->>\n";
	cout<<"EdgeID"<<"\t"<<"HopID"<<"\t"<<"Cost"<<"\n";
	for(i=0;i<no_serv;i++){
		if(myinfo.n_sno.compare(routeinfo[i].n_sno) !=0){
			if (routeinfo[i].n_cost >= 9999){
				cout<<routeinfo[i].n_sno<<" \t "<<routeinfo[i].n_next<<" \t "<<"inf";
			}
			else{
				cout<<routeinfo[i].n_sno<<" \t "<<routeinfo[i].n_next<<" \t "<<routeinfo[i].n_cost;
			}
			cout<<"\n";
		}
	}
}

void loadmsg(struct nodeinfo msginfo[],struct nodeinfo peerinfo[],string msg){
	//Code to load message to structure 
	int i,j;
	vector<string> value;
	vector<string> data_val;
	split(value,msg,is_any_of("|"));
	string getSNO(struct nodeinfo peerinfo[],string portno);
	for(i=0;i<value.size()-1;i++){
		split(data_val,value[i],is_any_of(" "));
		if(i==0){
			//handle first data different from rest
			msginfo[i].n_portno = data_val[1];
			msginfo[i].n_ip = data_val[2];
			msginfo[i].n_sno = getSNO(peerinfo, msginfo[i].n_portno);
		}
		else{
			//normal case for peer details of sender
			msginfo[i].n_ip = data_val[0];
			msginfo[i].n_portno = data_val[1];
			msginfo[i].n_sno = data_val[3];
			msginfo[i].n_cost = atoi(data_val[4].c_str()); 
		}
	}
/*	for(i=0;i<no_serv;i++){
		cout<<"Displaying data of msginfo here \n";
		cout<<" ip :"<<msginfo[i].n_ip<<" port :"<<msginfo[i].n_portno<<" sno :"<<msginfo[i].n_sno<<" cost :"<<msginfo[i].n_cost<<"\n";
		}*/
}
//*************************************Update DV logic starts here******************************************************** 
void updateDV(struct nodeinfo msginfo[] , struct nodeinfo peerinfo[],struct nodeinfo routeinfo[],struct mynodeinfo myinfo){
	int i,j;
	int cost_to_edge,cost_edge_to_next,cost_to_peer,cost_edge_to_peer;
	int msg_cos;
	cost_to_edge =0;cost_edge_to_next=0;cost_to_peer=0;cost_edge_to_peer=0;msg_cos=0;
	//calculate cost with the edge
	for(i=0;i<no_serv;i++){
		if(myinfo.n_sno.compare(routeinfo[i].n_sno) != 0){ 
			if(routeinfo[i].n_sno.compare(msginfo[0].n_sno)==0){
				cost_to_edge =routeinfo[i].n_cost;
			}
		}
	}
	for(i=0;i<no_serv;i++){
		if(myinfo.n_sno.compare(routeinfo[i].n_sno) != 0){ 
			for(j=1;j<no_serv;j++){
				if(routeinfo[i].n_sno.compare(msginfo[j].n_sno) ==0){
					if(routeinfo[i].n_cost >(cost_to_edge + msginfo[j].n_cost)){
						routeinfo[i].n_cost = cost_to_edge + msginfo[j].n_cost;
						routeinfo[i].n_next = msginfo[0].n_sno;
						//if the node is not edge then update mytable peerinfo 
						if(!peerinfo[i].isEdge){
							peerinfo[i].n_cost = cost_to_edge + msginfo[j].n_cost;
							peerinfo[i].n_next = msginfo[0].n_sno;
						}
					}
				}
			}
		}
	}
	//check for any updates in existing routing table (next hop) cause of an update in edge table
	for(i=0;i<no_serv;i++){
		if(routeinfo[i].n_next.compare(msginfo[0].n_sno)==0 && ! routeinfo[i].n_sno.compare(msginfo[0].n_sno)==0 ){
			// need to get the sno and the cost to sno from my table (peerinfo)
			for(j=0;j<no_serv;j++){
				if(peerinfo[j].n_sno.compare(routeinfo[i].n_sno)==0){
					cost_to_peer = peerinfo[j].n_cost;
				}
				if(msginfo[j].n_sno.compare(routeinfo[i].n_sno) ==0){
					cost_edge_to_peer = msginfo[j].n_cost;
				}
				//get cost to edge from my table
				if(peerinfo[j].n_sno.compare(msginfo[0].n_sno)==0){
					cost_to_edge = peerinfo[j].n_cost;
				}
			}
			//check if update to routing table is required(when the edge has been update)
			//even if my own table was updated this will correct my routing table
			if((cost_to_peer <= (cost_to_edge+ cost_edge_to_peer))&& peerinfo[i].isEdge){
				routeinfo[i].n_next = peerinfo[i].n_sno;
				routeinfo[i].n_cost = peerinfo[i].n_cost;
			}
			else{
				routeinfo[i].n_cost =cost_to_edge+ cost_edge_to_peer;
				//if this is not my edge update my table too
				if(!peerinfo[i].isEdge){
					peerinfo[i].n_cost = cost_to_edge+ cost_edge_to_peer;
				}
			}
		}
	}

}

bool updatePeerInfo(struct nodeinfo peerinfo[],struct nodeinfo routeinfo[],string edge_sno,string new_cost){

	//code to update my peer info table
	int newint_cost=0;
	int i,j;
	if(new_cost.compare("inf") ==0 ){
		newint_cost = 9999;
	}
	else {
		newint_cost = atoi(new_cost.c_str());
	}
	for(i=0;i<no_serv;i++){
		if (peerinfo[i].n_sno.compare(edge_sno)==0){
			if(peerinfo[i].isEdge){
				peerinfo[i].n_cost = newint_cost;
				if(routeinfo[i].n_sno.compare(routeinfo[i].n_next) ==0){
					routeinfo[i].n_cost = newint_cost;
				}
				if(peerinfo[i].isDisabled){
						for(j=0;j<no_serv;j++){
					//	cout<<routeinfo[i].n_sno<<"--------"<<routeinfo[j].n_next<<"\n";
						if(routeinfo[i].n_sno.compare(routeinfo[j].n_next) ==0){
							routeinfo[j].n_next = routeinfo[j].n_sno;
							if(peerinfo[j].isEdge){
							routeinfo[j].n_cost = peerinfo[j].n_cost;
							}
							else{
							 routeinfo[j].n_cost = 9999;
							}
							}
					}
				}
			}
			else{
				cout<<"ERROR :The Node Is Not Neighbour Cannot Update Cost:"<<peerinfo[i].n_sno<<"\n";
				return false;
			}
		}
	}
	//code to update next hops when node has got disabled
	return true;
}

string getSNO(struct nodeinfo peerinfo[],string portno){
	int i ;
	string sno;
	for(i=0;i<no_serv;i++){
		if(peerinfo[i].n_portno.compare(portno) ==0){
			sno = peerinfo[i].n_sno;
		}	
	}
	return sno;
}

void disableNode(struct nodeinfo peerinfo[], string d_sno){
	int i ;
	for(i=0;i<no_serv;i++){
		if(peerinfo[i].n_sno.compare(d_sno)==0){
			peerinfo[i].isDisabled = true;
		}
	}
}

void checkPeerState(struct nodeinfo peerinfo[],struct nodeinfo msginfo[]){
	int i,j;
	for(i=0;i<no_serv;i++){
		if(peerinfo[i].n_sno.compare(msginfo[0].n_sno) ==0){
			if(peerinfo[i].n_tval > 0){
				peerinfo[i].n_tval-=1;
			}
		}	
	}
}

void updatePeerStates(struct nodeinfo peerinfo[],struct nodeinfo routeinfo[],struct nodeinfo msginfo[]){
	int i,j;
	bool updatePeerInfo(struct nodeinfo peerinfo[],struct nodeinfo routeinfo[],string edge_sno,string cost);
	for(i=0;i<no_serv;i++){
		if(peerinfo[i].isEdge){
			peerinfo[i].n_tval +=1;
		}
		if(peerinfo[i].n_tval >3){
			//need to disable 
			peerinfo[i].isDisabled = true;
			updatePeerInfo(peerinfo,routeinfo,peerinfo[i].n_sno,"inf");	
		}
	}
}

void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

