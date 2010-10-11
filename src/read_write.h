
#ifndef _READ_WRITE_H_
#define _READ_WRITE_H_

#ifndef MY_MAJOR
#define MY_MAJOR 0
#endif

#ifndef MY_MINOR
#define MY_MINOR 0
#endif  

#ifndef NUM_DEVICES
#define NUM_DEVICES 1
#endif


/*
 * The rw device is a simple circular buffer. 
 */
#ifndef RW_BUFF_SIZE
#define RW_BUFF_SIZE 4000
#endif

 
/*
 * Name of the driver, writen to /proc/devices.
 */
char driver_name[] = "read_write_module";

extern int my_major;
extern int my_minor;
extern int num_devices; 


/* 
 * Each device will have the following data.
 */
struct rw_dev {
	wait_queue_head_t input_q, output_q; /* Read and write queues.        */
	char *buffer, *end;                  /* Befining and end of buffer.   */
	int buffer_size;                     /* Size of I/O buffer.           */
	char *read_ptr, *write_ptr;   	     /* Where to read and write from. */
	int num_readers, num_writers;	     /* Number of openings for r/w.   */
	struct semaphore sem;                /* Mutual exclusion semaphore    */
	struct cdev cdev;                /* Char device structure.        */

};


#endif /* _READ_WRITE_H_ */

