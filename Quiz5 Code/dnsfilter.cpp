//Harshank Patel
#include<arpa/inet.h>
#include<iostream>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<stdlib.h>
#include<unistd.h>
#include <cstring>
#include<string>

using namespace std;

int main(int argc, char* argv[]) {
    bool finds = false;
    int status;
    struct addrinfo hints, *p;
    string compString = argv[2];
    char *website = argv[1] ;
    struct addrinfo * servinfo; // will point to the results
    //preparing hints data structure
    memset( & hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    // look up the IP address from the name: "www.example.com"
    status = getaddrinfo(website, NULL, & hints, & servinfo);
    cout <<endl <<endl;
    for (p = servinfo; p != NULL; p = p -> ai_next) {
        void * addr;
        char * ipver;
        char ipstr[INET6_ADDRSTRLEN];
            // char dst[16] = {0};

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        
        if (p -> ai_family == AF_INET) {
            struct sockaddr_in * ipv4 = (struct sockaddr_in * ) p -> ai_addr;
            addr = & (ipv4 -> sin_addr);
            ipver = (char *)"IPv4";
        } 
        else {
            struct sockaddr_in6 * ipv6 = (struct sockaddr_in6 * ) p -> ai_addr;
            addr = & (ipv6 -> sin6_addr);
            ipver = (char *)"IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        string temp = ipstr;
        // printf("  %s: %s\n", ipver, ipstr);
        if (temp.find(compString) != std::string::npos) {
            std::cout <<"YES character found in this IP Address := " <<temp <<endl ;
        }
        else{
            std::cout <<"No character found here : " <<temp <<endl ;
        }
    }
    cout <<endl <<endl;
    freeaddrinfo(servinfo);
    return 0;
}
