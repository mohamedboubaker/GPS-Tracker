#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

uint8_t connect_packet[]= {
	0x10, // Packet type = CONNECT
	0x10, // Remaining length = 16
	0x00, 0x04, // Protocol name length  
	0x4d, 0x51, 0x54, 0x54, // Protocol name = MQTT
	0x04, // Protocol Version 
	0x02, // Connect flags
	0x00, 0x3c, // Keep alive time in secondes = 60 s
	0x00, 0x04, // Client ID length
	0x46, 0x46, 0x46, 0x46 // Client ID
};
	

uint8_t disconnect_packet[] = {
	0xe0, // Packet type = DISCONNECT
	0x00  // Remaining length = 0
};

uint8_t publish_packet[] ={
	0x30, // Packet type = Publish + DUP+QOS+retain=0
	0x0b, // Remaining length = 11
	0x00, 0x04, // Topic name length
	0x46, 0x46, 0x46, 0x46 // Topic name = Client ID
};
uint8_t payload[]={0x68, 0x65, 0x6c, 0x6c, 0x6f}; // Payload	


void print_array(uint8_t * a,uint8_t n){
	uint8_t i=0;
	for(i=0;i<n;i++)
		printf("%02x ",a[i]);
	printf("\n");
}



int main(){
	short server_socket = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in remote = {0};
	remote.sin_addr.s_addr = inet_addr("127.0.0.1");
	remote.sin_family = AF_INET;
	remote.sin_port = htons(1883);

	print_array(connect_packet,18);
	
	connect(server_socket,(struct sockaddr *)&remote,sizeof(struct sockaddr_in));
	send(server_socket,connect_packet,18,0);
	sleep(1);
	send(server_socket,publish_packet,13,0);	
	send(server_socket,payload,5,0);	
	sleep(1);
	send(server_socket,disconnect_packet,2,0);
	close(server_socket);
		

return 0;
}



