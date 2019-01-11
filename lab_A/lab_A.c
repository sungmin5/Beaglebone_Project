#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<getopt.h>
#include<string.h>
#include<math.h>
#include<poll.h>
#include<time.h>
#include<fcntl.h>
#include<signal.h>
#include<mraa/aio.h>
#include<mraa/gpio.h>

//global variables
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k

int old_sec = -60;
//int time_first = 1;
int period = 1;
int opt_log = 0;
int log_fd; 
int is_stop = 0;
int is_Fahrenheit = 1; //by default
mraa_aio_context temp;
mraa_gpio_context button;


void close_all(){
	mraa_aio_close(temp);
	mraa_gpio_close(button);
	if(opt_log)
	   close(log_fd);
}//close_all

void init_sensor(){
	temp = mraa_aio_init(1);
	if(temp == NULL ){
           fprintf(stderr, "ERROR: initializing temperature\n");
           exit(1);
        }

	button = mraa_gpio_init(62);
	if( button == NULL){
	   fprintf(stderr, "ERROR: button initializing\n");
	   exit(1);
	}
	//configure button GPIO interface to be input pin
	mraa_gpio_dir(button, MRAA_GPIO_IN);
}//init_sensor

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
        printf("%s SHUTDOWN\n", buffer);

        //write to log if user asks for it
        if(opt_log)
	   dprintf(log_fd,"%s SHUTDOWN\n", buffer);
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
	   	printf("%s %0.1f\n", buffer, temperature);

	   	//write to log if user asks for it
	   	if(opt_log)
	           dprintf(log_fd, "%s %0.1f\n", buffer, temperature);
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
	ssize_t read_ret = read(0, command, 256);
	int i;
	int start = 0; //keeps track of beginning of command
	for(i = 0; i < read_ret; i++){
	   if(command[i] == '\n'){
		command[i] = 0;
		if(strcmp(command+ start, "OFF") ==0){
		   printf( "OFF\n");
                   if(opt_log) dprintf(log_fd, "OFF\n");
		   end_sampling();
		}
                else if(strcmp(command + start, "START") ==0){
                   printf( "START\n");
		   if(opt_log)dprintf(log_fd, "START\n");
		   is_stop = 0;
                }
                else if(strcmp(command+ start, "STOP") ==0){
                   printf("STOP\n");
                   if(opt_log)dprintf(log_fd, "STOP\n");
		   is_stop = 1;
                }
		else if(strcmp(command+ start, "LOG") ==0){
                   printf( "LOG\n");
                   if(opt_log)dprintf(log_fd, "LOG\n");
                }
		else if(strcmp(command+ start, "SCALE=F") ==0){ 
		   is_Fahrenheit = 1;
                   printf( "SCALE=F\n");
                   if(opt_log) dprintf(log_fd, "SCALE=F\n");
		}
		else if(strcmp(command+ start, "SCALE=C") ==0){
		   is_Fahrenheit = 0;
                   printf( "SCALE=C\n");
                   if(opt_log)dprintf(log_fd, "SCALE=C\n");
                }
                else if(strncmp(command + start, "PERIOD=", 7) ==0){
                   printf( "PERIOD=%c\n", command[i-1]);
                   if(opt_log) dprintf(log_fd, "PERIOD=%c\n", command[i-1]);
		   period = (int)command[i-1];
		}
		else
		   continue;
		start = (i+1);
	   }
	}
}//get_stdin




int main(int argc,  char *argv[]){
	int opt;
	char *log_file;
	static struct option long_options[] = {
	   {"period", required_argument, 0, 'p'},
	   {"scale", required_argument, 0, 's'},
	   {"log", required_argument, 0, 'l'},
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
              default:
		 fprintf(stderr, "ERROR: Usage: ./lab4b [--period=#] [--scale={C|F}] [--log=filename]\n" );
                 exit(1);			
 	   }
	}//end while

  	/*when --log option is given*/
  	if(opt_log){
    	   log_fd = creat(log_file, S_IRWXU);
    	   if(log_fd < 0){
              fprintf(stderr, "ERROR: creat() file failure\n");
              exit(1);
    	   }	
  	}
	
	//initialize sensors
	init_sensor();

	//poll set up
	int pollret;
	struct pollfd poll_stdin[1];
	poll_stdin[0].fd = STDIN_FILENO;
	poll_stdin[0].events = POLLIN | POLLHUP | POLLERR;

	while(1){
	   pollret= poll(poll_stdin,1,0);
	   if(pollret < 0){
		fprintf(stderr, "ERROR: poll error\n");
		exit(1);
	   }
	   
	   //check button	   
	   mraa_gpio_isr(button,MRAA_GPIO_EDGE_RISING, &end_sampling, NULL) ;
	
	   //get and print temperature with time
	   print_temperature();

	   if(poll_stdin[0].revents & POLLIN){
		get_stdin();
	   }
	   if(poll_stdin[0].revents & (POLLHUP|POLLERR)){
		fprintf(stderr, "ERROR: polling stdin failure\n");
		close_all();
		exit(1);
	   }
	}
	close_all();
	exit(0);
}
