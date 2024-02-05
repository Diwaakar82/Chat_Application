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
#include <signal.h>

#define PORT 8080			//Port used for connections
#define BACKLOG 10			//Pending connections queue can hold
#define MAX_CLIENTS 20		//Maximum connectable clients
#define BUFFER_SIZE 1000	//Maximum buffer size    

int userid = 1000;
static _Atomic int client_cnt = 0;

typedef struct client_details
{
	int userid, sockfd;
	char name [20];
	struct sockaddr_in address;
} client_t;

client_t *clients [MAX_CLIENTS];
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

//Receive message
int receive_message (int sockfd, char *buf)
{
	int numbytes;
	memset (buf, 0, 1000);
	
	if ((numbytes = recv (sockfd, buf, 1000, 0)) == -1)
		perror ("recv");

	return numbytes;
}

//Add client to list
void queue_add (client_t *client)
{
	pthread_mutex_lock (&clients_mutex);
	
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!clients [i])
		{
			clients [i] = client;
			break;
		}
	}
	
	pthread_mutex_unlock (&clients_mutex);
}

//Remove client from list
void queue_remove (int userid)
{
	pthread_mutex_lock (&clients_mutex);
	
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients [i] -> userid == userid)
		{
			clients [i] = NULL;
			break;
		}
	}
	
	pthread_mutex_unlock (&clients_mutex);
}

void send_message (char *msg, int userid)
{
	pthread_mutex_lock (&clients_mutex);
	
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (clients [i] && clients [i] -> userid != userid)
			if (send (clients [i] -> sockfd, msg, 1000, 0))
			{
				perror ("ERROR: write failed.");
				break;
			}
	
	pthread_mutex_unlock (&clients_mutex);
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

//Perform client operations
void *handle_client (void *arg)
{
	char buffer [BUFFER_SIZE];
	char name [20];
	int leave_flag = 0;
	
	client_cnt++;
	client_t *client = (client_t *)arg;
	
	if (recv (client -> sockfd, name, 20, 0) <= 0)
	{
		printf ("Didn't enter name.");
		leave_flag = 1;
	}
	else
	{
		strcpy (client -> name, name);
		sprintf (buffer, "%s has joined the chat!!!\n", name);
		printf ("%s\n", buffer);
		send_message (buffer, client -> userid);
	}
	
	bzero (buffer, BUFFER_SIZE);
	
	while (1)
	{
		if (leave_flag)
			break;
			
		int numbytes = recv (client -> sockfd, buffer, BUFFER_SIZE, 0);
		
		if (numbytes > 0 && strlen (buffer) > 0)
		{
			send_message (buffer, client -> userid);
			
			str_trim (buffer, sizeof (buffer));
			printf ("%s -> %s\n", buffer, client -> name);
		}
		else if (numbytes == 0 || strstr (buffer, "exit"))
		{
			sprintf (buffer, "%s has left!!!\n", client -> name);
			printf ("%s", buffer);
			send_message (buffer, client -> userid);
			leave_flag = 1;
		}
		else
		{
			printf ("ERROR: -1\n");
			leave_flag = 1;
		}
		
		bzero (buffer, BUFFER_SIZE);
	}
	
	//Delete exited client information and free thread
	close (client -> sockfd);
	queue_remove (client -> userid);
	free (client);
	client_cnt--;
	pthread_detach (pthread_self ());
	
	return NULL; 
}

//Connect to a socket
struct sockaddr_in connect_to_socket (int *sockfd)
{
	int yes = 1;
 	struct sockaddr_in server_addr;
 	
	if ((*sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("server: socket");
		exit (0);
	}
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr ("127.0.0.1");
	server_addr.sin_port = htons (PORT);
	
	if (setsockopt (*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1)
	{
		perror ("setsockopt");
		exit (1);
	}
	if (bind (*sockfd, (struct sockaddr*)&server_addr, sizeof (server_addr)) < 0)
	{
		close (*sockfd);
		perror ("server: bind");
		exit (0);
	}
	
	return server_addr;
}

int main ()
{
	int sockfd, new_fd;
	struct sockaddr_in server_addr, client_addr;
	pthread_t tid;
	socklen_t sin_size;
	char s [INET6_ADDRSTRLEN], IP [INET6_ADDRSTRLEN];
	int rv;
	
	server_addr = connect_to_socket (&sockfd);

	//Failed to listen
	if (listen (sockfd, BACKLOG) == -1)
	{
		perror ("listen");
		exit (1);
	}
	
	printf ("Server: waiting for connection: \n");
	
	while (1)
	{
		socklen_t client_len = sizeof (client_addr);
        char buffer [1000], buff [1000];
        
		new_fd = accept (sockfd, (struct sockaddr *)&client_addr, &client_len);
		if (new_fd == -1)
		{
			perror ("accept");
			continue;
		}
		
		//Maximum client count reached
		if ((client_cnt + 1) == MAX_CLIENTS)
		{
			printf ("Maximum clients reached. Can't connect!!!\n");
			printf ("Rejected: %s: %d\n", inet_ntoa (client_addr.sin_addr), client_addr.sin_port);
			close (new_fd);
			continue;
		}
		
		client_t *client = (client_t *)malloc (sizeof (client_t));
		client -> address = client_addr;
		client -> sockfd = new_fd;
		client -> userid = userid++;
		
		queue_add (client);
		pthread_create (&tid, NULL, &handle_client, (void *)client);
		
		sleep (1);
	}
	close (sockfd);
	
	return 0;
}
