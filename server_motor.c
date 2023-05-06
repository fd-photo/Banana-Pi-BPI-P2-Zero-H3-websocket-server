#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <pthread.h>
#include "gpio.h"

// gcc server_motor.c gpio.c -o server_motor
// работает с екодером 
// сделать выбор стороны вращения 
// роегулировку скорости 

#define PORT 1985
#define IP_A "192.168.0.17" 

bool stop = false;

volatile int count = 0;  // Declare count as volatile

// --------------------------------------

void* Motor_Pwm(void *arg) {
    //bool stop = false;
    GPIO_Init();
    GPIO_ConfigPin(PA, 18, OUT);

    while (!stop) {
        GPIO_SetPin(PA, 18, true);
        usleep(5);
        GPIO_SetPin(PA, 18, false);
        usleep(5);
    }

    pthread_exit(NULL);
}
// ---------------------------------------

int getCount() {
    return count;
}

void pollGpio(char * steps) {

    // Export GPIO 20 and set direction and edge
    int fd_export = open("/sys/class/gpio/export", O_WRONLY);
    write(fd_export, "20", 2);
    close(fd_export);

    int fd_direction = open("/sys/class/gpio/gpio20/direction", O_WRONLY);
    write(fd_direction, "in", 2);
    close(fd_direction);

    int fd_edge = open("/sys/class/gpio/gpio20/edge", O_WRONLY);
    write(fd_edge, "both", 4);
    close(fd_edge);

    // Initialize interrupt counter and last value
    int lastValue;
    char buf[2];

    // Open GPIO value file for reading
    int fd_value = open("/sys/class/gpio/gpio20/value", O_RDONLY);

    // Loop to read GPIO value and count interrupts
    while (1) {

        struct pollfd poll_fd;
        poll_fd.fd = fd_value;
        poll_fd.events = POLLPRI;

        // Wait for interrupt
        poll(&poll_fd, 1, -1);

        // Read GPIO value and check if it changed
        lseek(fd_value, 0, SEEK_SET);
        read(fd_value, buf, 2);
        int value = atoi(buf);

        if (value != lastValue) {
            count++;  // Increment counter
            //printf("GPIO 20 state changed! Interrupt count: %d\n", count);  // Print message and counter

            if (count >= atoi(steps)) // сделать проверку числа
            {
                    stop = true;
		    count = 0;
		    break;
            }
            lastValue = value;  // Update last value
        }
    }

    // Close file descriptor and unexport GPIO on program exit
    close(fd_value);
    fd_export = open("/sys/class/gpio/unexport", O_WRONLY);
    write(fd_export, "20", 2);
    close(fd_export);
}

// --------------------------------------

int main() {
    int server_fd, new_socket, valread, rc;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char* hello = "Привет, я сервер!";
    
//    pthread_t thread;  

    // Создание серверного сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Ошибка создания серверного сокета");
        exit(EXIT_FAILURE);
    }
  
    // Установка опций для серверного сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Ошибка установки опций для серверного сокета");
        exit(EXIT_FAILURE);
    }
  
    // Настройка адреса сервера
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr ( IP_A );
    address.sin_port = htons( PORT );
      
    // Связывание серверного сокета с адресом и портом
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("Ошибка связывания серверного сокета с адресом и портом");
        exit(EXIT_FAILURE);
    }
    
    // Ожидание подключения клиента
    if (listen(server_fd, 5) < 0) {
        perror("Ошибка при попытке прослушивания серверного сокета");
        exit(EXIT_FAILURE);
    }
  
    while(1) {
        // Принятие подключения от клиента
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("Ошибка при принятии подключения от клиента");
            exit(EXIT_FAILURE);
        } 
        // Отправка сообщения клиенту
        send(new_socket, hello, strlen(hello), 0);
      
        // Цикл приёма сообщений от клиента
        while(1) 
        {
            memset(buffer, 0, sizeof(buffer));
            valread = read( new_socket , buffer, 1024);
            if(valread < 0) 
            {
                perror("Ошибка при чтении сообщения от клиента");
                exit(EXIT_FAILURE);
            }
            printf("Сообщение от клиента: %s\n",buffer );
//---------------------------------------------------------------	    
            //if (strcmp(buffer,"1000") == 0)
	    if (buffer > 0 )
            {
                printf("Сообщение: %s\n",buffer );
		////////////////////////
		pthread_t thread;
		rc = pthread_create(&thread, NULL, Motor_Pwm, NULL);
		if (rc) {
      		    printf("Error creating thread: %d\n", rc);
       	            return -1;
		}

		
		pollGpio(buffer);
		printf("Сообщение от клиента: %d\n",getCount() );
		
		/////////////////////////
		//usleep(1000000);
		//printf("Сообщение от клиента: %d\n",stop );
		//stop = true;
		//printf("Сообщение от клиента: %d\n",stop );
		/////////////////////////
		
		// Завершаем работу потока
		void *status;
    		rc = pthread_join(thread, &status);
       		if (rc) {
        	    printf("Error joining thread: %d\n", rc);
        	    return -1;
                }
		stop = false;

            }
//---------------------------------------------------------------            
        }
    }
    return 0;
}
