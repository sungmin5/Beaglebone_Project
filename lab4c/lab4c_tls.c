/*
Name: Sung Min Ahn
EMAIL: sungmin.sam5@gmail.com
UID:604949854
*/

#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<getopt.h>
#include<string.h>
#include<math.h>
#include<poll.h>
#include<time.h>
#include<fcntl.h>
#include<signal.h>
#include<mraa/aio.h>
#include<mraa/gpio.h>
#include<sys/time.h>
#include<sys/socket.h>
#include<netdb.h>
#include<netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

//global variables
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k

//global variables for commands
int old_sec = -60; 
int period = 1;
int is_stop = 0;
int is_Fahrenheit = 1; //Fahrenheit by default

//main option and file descriptors
int opt_log = 0;
FILE *log_fd;
int sockfd;

//SSL
SSL *ssl_client;

//sensors
mraa_aio_context temp;
mraa_gpio_context button;

void close_all(){
	mraa_aio_close(temp);
	mraa_gpio_close(button);
	close(sockfd);
}//close_all

void init_sensor(){
	temp = mraa_aio_init(1);
	if(temp == NULL ){
           fprintf(stderr, "ERROR: initializing temperature\n");
           exit(2);
        }

	button = mraa_gpio_init(60);
	if( button == NULL){
	   fprintf(stderr, "ERROR: button initializing\n");
	   exit(2);
	}
	//configure button GPIO interface to be input pin
	mraa_gpio_dir(button, MRAA_GPIO_IN);
}//init_sensor

void init_ssl_client(){
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
	SSL_CTX *new_context = SSL_CTX_new(TLSv1_client_method());
	if(new_context == NULL){
		fprintf(stderr, "ERROR: SSL new context error\n");
		exit(2);
	}
	ssl_client = SSL_new(new_context);
	if(ssl_client == NULL){
		fprintf(stderr, "ERROR: SSL new client error\n");
		exit(2);
	}
}//init_ssl_client

void end_sampling(){
	//get localtime
	struct tm *info;
	time_t now;
	time(&now);
	info = localtime(&now);

	//write time in HH:MM:SS format
	char buffer[20];
	strftime(buffer, 20, "%X", info);

	//write to stdout time and temperature
	char time_string[20];
	sprintf(time_string, "%s SHUTDOWN\n", buffer);
        SSL_write(ssl_client, time_string, strlen(time_string));

        //write to log if user asks for it
        if(opt_log)
	   fprintf(log_fd,"%s SHUTDOWN\n", buffer);
	//close mraa objects and fd's
	close_all();
	exit(0);
}//end_sampling


void print_temperature(){
	/**
	get time first and if it is the correct time(which depends on period) do
	the calculation
	**/
	//get localtime
	struct tm *info;
	time_t now;
	time(&now);
	info = localtime(&now);
	
	//printf("second %d\n", old_sec + period);
	//if the correct time
	if(info->tm_sec  == (old_sec + period)%60 || old_sec == -60){	
  	   double temperature;
           //get temperature
           int get_temp_from_sensor = mraa_aio_read(temp);

           double R = 1023.0/get_temp_from_sensor-1.0;
           R = R0*R;

           temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15;
           if(is_Fahrenheit)//change to fahrenheit
        	temperature = temperature * 9/5 +32;

	   //get Time representation (HH:MM:SS)
	   char buffer[20];
	   strftime(buffer, 20,"%X", info);
	   
	   //check whether STOP command is given
	   if(!is_stop){
		//write to stdout time and temperature
		char time_n_temp[30];
		sprintf(time_n_temp, "%s %0.1f\n", buffer, temperature);
	   	SSL_write(ssl_client, time_n_temp, strlen(time_n_temp)) ;//print out

	   	//write to log if user asks for it
	   	if(opt_log)
	           fprintf(log_fd, "%s %0.1f\n", buffer, temperature);//log
	   }
	   if(old_sec == -60)
	  	old_sec = info->tm_sec;
	   else
	   old_sec += period;
	}
}//print_temperature


void get_stdin(){
	//read from stdin
	char command[256];
	ssize_t read_ret = SSL_read(ssl_client, command, 256);
	if(read_ret < 0){
	   fprintf(stderr, "ERROR: read from server failed");
	   exit(2);
	}

	int i;
	int start = 0; //keeps track of beginning of command
	for(i = 0; i < read_ret; i++){
	   if(command[i] == '\n'){	
		command[i] = 0;
		if(strcmp(command + start, "OFF") ==0){
		if(opt_log) {
		      fprintf(log_fd, "OFF\n");
		      fflush(log_fd);
		   }
		   end_sampling();
		}
                else if(strcmp(command + start, "START") ==0){
		   is_stop = 0;
		   if(opt_log){
		      fprintf(log_fd, "START\n");
		      fflush(log_fd);
		   }		 
                }
                else if(strcmp(command+ start, "STOP") ==0){
                   if(opt_log){
		      fprintf(log_fd, "STOP\n");
		      fflush(log_fd);
		   }
		   is_stop = 1;
                }
		else if(strncmp(command+ start, "LOG", 3) ==0){
                   if(opt_log){
		      fprintf(log_fd, "%s\n", command + start);
		      fflush(log_fd);
		   }
                }
		else if(strcmp(command+ start, "SCALE=F") ==0){ 
		   is_Fahrenheit = 1;
                   if(opt_log){
		      fprintf(log_fd, "SCALE=F\n");
		      fflush(log_fd);
		   }
		}
		else if(strcmp(command+ start, "SCALE=C") ==0){
		   is_Fahrenheit = 0;
                   if(opt_log){
		      fprintf(log_fd, "SCALE=C\n");
		      fflush(log_fd);
		   }
                }
                else if(strncmp(command + start, "PERIOD=", 7) == 0){
                   if(opt_log){
		      fprintf(log_fd, "PERIOD=%c\n", command[i-1]);
		      fflush(log_fd);
		   }
		   period = atoi(&command[i-1]);
		}
		else
		   continue;
		start = (i+1);
	   }
	}
}//get_stdin


int main(int argc,  char *argv[]){
	int id = -1;
	int opt;
	char *log_file;
	char *host_name;
	int opt_id = 0;
	int opt_host = 0;
	
	static struct option long_options[] = {
	   {"period", required_argument, 0, 'p'},
	   {"scale", required_argument, 0, 's'},
	   {"log", required_argument, 0, 'l'},
	   {"id", required_argument, 0, 'i'},
	   {"host", required_argument, 0, 'h'},
	   {0, 0, 0, 0}
	};

	while((opt = getopt_long(argc, argv, "p:s:l", long_options, NULL)) != -1) {
	   switch(opt) {
	      case 'p':
	         period = atoi(optarg);
		 if(period < 0){
		    fprintf(stderr, "ERROR: period must be non-negative value\n");
	            exit(1);
		 }
	         break;
	      case 's':
	         if(strlen(optarg)==1){
		    if(optarg[0] =='C')
			is_Fahrenheit = 0;
		    else if(optarg[0] == 'F');
		    else{
			fprintf(stderr, "ERROR: --scale={C|F}\n");
		        exit(1);
		    }
                 }
	         else{
 		    fprintf(stderr, "ERROR: --scale={C|F}\n");
		    exit(1);
		 }
                 break;
	      case 'l':
		 opt_log=1;
		 //get log file name
		 log_file = optarg;
		 break;
	      case 'i':
	         if(strlen(optarg) != 9){
		    fprintf(stderr, "ERROR: --id=9digitnumber\n");
		    exit(1);
		 }
		 else{
		    id = atoi(optarg);
		    opt_id = 1;
		 }
	         break;
	      case 'h':
	         host_name = optarg;
		 opt_host = 1;
		 break;
              default:
		 fprintf(stderr, "ERROR: Usage: ./lab4b [--period=#] [--scale={C|F}] [--log=filename] [--id=9DigitNumber] [--host=hostname/address]\n" );
                 exit(1);			
 	   }
	}//end while

	//get non switch-parameter port number
	//opt_get_long reorders the parameter
 	int port_num;
  	if(argv[optind] =='\0' || argv[optind] == NULL){
	   fprintf(stderr, "ERROR: --port option is a mandatory option");
	   exit(1);
	}else
	   port_num = atoi(argv[optind]);
	

	/*when --log option is given*/
  	if(opt_log){
    	   log_fd = fopen(log_file, "w");
    	   if(log_fd == NULL){
              fprintf(stderr, "ERROR: creat() file failure\n");
              exit(1);
    	   }
	}

	if((!opt_log) & (!opt_host) & (!opt_id)){
	   fprintf(stderr, "ERROR: [--log=filename] [--host=hostname] [--id=9digitNumber] aremandatory options\n");
	   exit(1);
	}
	
	//socket setup
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	sockfd = socket(AF_INET, SOCK_STREAM,0);
	if(sockfd < 0){
	   fprintf(stderr, "ERROR: socket failure\n");
	   exit(1);
	}
	
	server = gethostbyname(host_name);
	if(server == NULL){
	   fprintf(stderr, "ERROR: server open failure");
	   exit(1);
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char *)&serv_addr.sin_addr.s_addr,(char *)server->h_addr, server->h_length);
	serv_addr.sin_port = htons(port_num);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
	    fprintf(stderr, "ERROR: connecting to server failure\n");
	    exit(2);
	}
	//SSL setup
	init_ssl_client();
	if(!SSL_set_fd(ssl_client, sockfd)){
		fprintf(stderr, "ERROR: SSL file descriptor error");
		exit(2);
	}

	int ssl_check = SSL_connect(ssl_client);
	if(ssl_check != 1){
		fprintf(stderr, "ERROR: SSL connection failure\n");
		exit(2);
	}


	//report ID number
	char id_buf[13];
	sprintf(id_buf, "ID=%d\n", id);
	SSL_write(ssl_client, id_buf, strlen(id_buf));
	if(opt_log){
	   fprintf(log_fd, "ID=%d\n", id);
	   fflush(log_fd);
	}

	


	//initialize sensors
	init_sensor();	   

	//poll set up
	int pollret;
	struct pollfd poll_stdin[1];
	poll_stdin[0].fd = sockfd;
	poll_stdin[0].events = POLLIN;
	while(1){
	   pollret= poll(poll_stdin,1,0);
	   if(pollret < 0){
		fprintf(stderr, "ERROR: poll error\n");
		exit(2);
	   }
	   
	   //check button	   
	   mraa_gpio_isr(button,MRAA_GPIO_EDGE_RISING, &end_sampling, NULL) ;
	
	   //get and print temperature with time
	   print_temperature();

	   if(poll_stdin[0].revents & POLLIN){
	       get_stdin();
	   }
	}//end while
	close_all();
	exit(0);
}
