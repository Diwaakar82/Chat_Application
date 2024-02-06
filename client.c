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

//Receive message
void receive_message ()
{
	char message [LENGTH];
	
	while (1)
	{
		int numbytes = recv (sockfd, message, LENGTH, 0);
		
		if (numbytes > 0)
		{
			if (strstr (message, "done"))
				break;
			printf ("%s\n", message);
		}
		else if (numbytes == 0)
			break;
		
		memset (message, 0, LENGTH);
	}
}


//Send message
void send_message ()
{
	char msg [LENGTH];
	char new_name [20];
	char buffer [2 * LENGTH];
	
	while (1)
	{
		int choice, exit = 0;
		printf ("\n1. Update name.\n2. List active users\n3. Send message to user.\n4. Exit\nWhat do you want to do?\n");
		scanf ("%d", &choice);
		
		switch (choice)
		{
			case 1:
				printf ("Enter new name: ");
				scanf ("%s", new_name);
				
				sprintf (buffer, "1%s", new_name);
				send (sockfd, buffer, strlen (buffer), 0);
				strcpy (name, new_name);
				break;
				
			case 2:
				buffer [0] = '2';
				buffer [1] = '\0';
				
				send (sockfd, buffer, strlen (buffer), 0);
				receive_message ();
				break;
				
			case 3:
				printf ("Enter whom to message: ");
				scanf ("%s", new_name);
				
				getchar ();
				printf ("Enter message: ");
				fgets (msg, LENGTH, stdin);
				str_trim (msg, LENGTH);
				
				sprintf (buffer, "3%s %s: %s\n", new_name, name, msg);
				send (sockfd, buffer, strlen (buffer), 0);
				break;
				
			case 4:
				exit = 1;
		}
		
		if (exit)
			break;
			
/*		fgets (msg, LENGTH, stdin);*/
/*		str_trim (msg, LENGTH);*/
/*		*/
/*		if (strstr (msg, "exit"))*/
/*			break;*/
/*		else*/
/*		{*/
/*			sprintf (buffer, "%s: %s\n", name, msg);*/
/*			send (sockfd, buffer, strlen (buffer), 0);*/
/*		}*/
		
		bzero (new_name, 20);
		bzero (msg, LENGTH);
		bzero (buffer, LENGTH + 20);
	}
	flag = 1;
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
