#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>

#define PORT 8080			//Port used for connections
#define LENGTH 1000

sig_atomic_t flag = 0;
int sockfd = 0;
char name [20];

//Print > at start of line
void str_overwrite_stdout ()
{
	printf ("> ");
	fflush (stdout);
}

//Trim to one line
void str_trim (char* str, int length) 
{
  	int i;
  	for (i = 0; i < length; i++) 
  	{
    	if (str [i] == '\n') 
    	{
      		str [i] = '\0';
      		break;
    	}
  	}
}

//Send message
void send_message ()
{
	char msg [LENGTH];
	char buffer [LENGTH + 22];
	
	while (1)
	{
		str_overwrite_stdout ();
		fgets (msg, LENGTH, stdin);
		str_trim (msg, LENGTH);
		
		if (strstr (msg, "exit"))
			break;
		else
		{
			sprintf (buffer, "%s: %s\n", name, msg);
			send (sockfd, buffer, strlen (buffer), 0);
		}
		
		bzero (msg, LENGTH);
		bzero (buffer, LENGTH + 20);
	}
	flag = 1;
}

//Receive message
void receive_message ()
{
	char message [LENGTH];
	
	while (1)
	{
		int numbytes = recv (sockfd, message, LENGTH, 0);
		
		if (numbytes > 0)
		{
			printf ("%s\n", message);
			str_overwrite_stdout ();
		}
		else if (numbytes == 0)
			break;
		
		memset (message, 0, LENGTH);
	}
}

int main ()
{
	char *ip = "127.0.0.1";
	struct sockaddr_in server_addr;
	
	printf ("Enter your name: ");
	fgets (name, 20, stdin);
	str_trim (name, strlen (name));
	
	sockfd = socket (AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr (ip);
	server_addr.sin_port = htons (PORT);
	
	if (connect (sockfd, (struct sockaddr *)&server_addr, sizeof (server_addr)) < 0)
	{
		printf ("ERROR: connect\n");
		return 0;
	}
	
	//Send client name
	send (sockfd, name, 20, 0);
	
	printf ("=== WELCOME TO THE CHATROOM ===\n");
	
	pthread_t send_msg_thread;
	if (pthread_create (&send_msg_thread, NULL, (void *) send_message, NULL) != 0)
	{
		printf ("ERROR: pthread for send\n");
		return 0;
	}
	
	pthread_t recv_msg_thread;
	if (pthread_create (&recv_msg_thread, NULL, (void *) receive_message, NULL) != 0)
	{
		printf ("ERROR: pthread for receive\n");
		return 0;
	}
	
	while (1)
	{
		if (flag)
		{
			printf ("Bye!!\n");
			break;
		}
	}
	
	close (sockfd);
 	return 0;
}
