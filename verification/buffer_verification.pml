/* PROMELA model to verify the behaviour of the circular LIFO buffer */

/* defines how many write calls will be simulated */
#define maxDataCounter  10;


/* Variables for algorithm */

/* pointer to the entry that was written last and is most current */
byte lw=0;
/* pointer to the entry that was read last or is read at the moment */
byte lr=0;

bool readInProgress=false;


int data[3];

/* Variables for verification */

/* counter simulating new data */
byte dataCounter=1;
/* last value that was read successful */
byte lastDataRead=0;


/* process simulating writing of the buffer */
proctype write()
{
/* reduces the state space, by terminating after maxDataCounter was reached */
do
	:: dataCounter < maxDataCounter ->
		/* simulate more current data */
		dataCounter=dataCounter+1;	
		
		/* determine where the new value can safely be inserted into the buffer without
		risking a read write conflict or overwriting the newest value, that is ready to be read */
		byte nw=(lw+1)%3;
		byte lw_temp = lw;		
		byte lr_temp = lr;
		
		do
			:: (nw == lr_temp || nw ==lw_temp) -> 
				nw=(nw+1)%3;
				lw_temp = lw;				
				lr_temp = lr;
				
								
			:: (!(nw == lr_temp || nw ==lw_temp))  -> break;
		od
		
		/* assert that the new write pointer does not point to a buffer position
 		that is read at the moment */
		assert(!(nw==lr && readInProgress));
		/* assert that the value is within the valid range for the buffer */
		assert(nw>=0 && nw<3);
		/* put new data into the buffer */
		data[nw]=dataCounter;
		
		/* indicate that this is the position with the most current value */
		lw=nw;

	/* terminate, after final value was written */
	:: dataCounter == maxDataCounter -> break;
od
}

/* process simulating reading of the buffer */
proctype read()
{
	/* reduces the state space, by terminating after maxDataCounter was reached */
do
	:: lastDataRead < maxDataCounter ->
		/* block if no new data is available
		Remark: In the real program, the function may also return "no new value", so the execution may proceed with the old values */
		(lw != lr);
		
		/* set the read pointer to the value that is most current at the moment */
		/* the assignment of lr=lw uses a temp variable, to account for the fact, that assigning a
		atomic variable to the value of another one is NOT atomic, but uses the two sperate atomic operations
		load and store */
		byte lw_temp=lw;
		lr=lw_temp;
		

		do
			:: lw_temp != lw ->					
				lw_temp=lw;
				lr=lw_temp;
				
			:: lw_temp == lw -> break; 
		od

		/* ensure that the data that was read is newer than the last value that was read */
		readInProgress=true;
		assert(data[lr] > lastDataRead);
		lastDataRead=data[lr];
		readInProgress=false;
		
	/* terminate, after final value has been read */
	:: lastDataRead == maxDataCounter -> break;
od
}

/* process checking invariances */
proctype monitor()
{
	do
		::assert(lw>=0 && lw<3);
		::assert(lr>=0 && lr<3);
	od
}

/* initialization sequence */
init
{
	/* initializing data */
	data[0]=-1;
	data[1]=-1;
	data[2]=-1;
	
	/* starting processes*/
	run monitor()
	run read()
	run write()
}
