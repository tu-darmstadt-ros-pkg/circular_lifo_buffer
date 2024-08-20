#  Lock-Free Circular LIFO Buffer

## General
This is the implementation of a lock-free circular LIFO buffer in C++.
It is designed such that one thread can produce new data while another thread can retrieve the data without blocking during the read or write process.

As the  circular buffer uses three slots, it is ensured that even if reading and writing is always done in parallel, the reader thread can access the newest data for which the write process has been finished. 
This is particularly of use in real-time applications. We utilized this functionality e.g. in our hardware interface to ensure that the robot's controllers and the hardware communication can not get blocked by each other.

## Usage
### Documentation
For a detailed documentation refer to the Doxygen pages.
When building the package you can use the flag '-DBUILD_DOC=TRUE' to build the documentation. You can access it in the doc folder afterwards.

### Examples
For simplicity we used integer as type in the examples, but you can use complex types like structs and classes, too.

Using simplified API:
```c++
/* the setup can be done by any thread */

// instantiate the buffer
CircularLifoBuffer<int> buffer;

/* writing is performed by thread A */
// push new value into the buffer
int input_value = 7;
buffer.push(input_value);

/* all the read operations are performed by thread B */
// check if the buffer has new data
bool has_new_data = buffer.hasNewData();

// has_new_data is true as a value was inserted but not read yet

// check if the buffer has new data and extract it
int newest_data = 0;
has_new_data = basic_buffer.popIfNew(newest_data);

std::cout << newest_data << std::endl; // newest data should now be 7

// reset newest data
newest_data = 0;

has_new_data = basic_buffer.popIfNew(newest_data);

// has_new_data is now false
// newest data should now still be 0 as no new element has been inserted since the last extraction
std::cout << newest_data << std::endl; 

has_new_data = basic_buffer.pop(newest_data);

// newest data should now be 7 again, but has_new_data is still false 
std::cout << newest_data << std::endl;
```
Using API designed for avoiding memory copies:

```c++
/* setup can be done by any thread */
// instantiate the buffer
CircularLifoBuffer<int> buffer;

/* write access by thread A */
int input_value = 7;

// retrieve a new write access pointer
int* const write_ptr = advanced_buffer.getWriteAccessPtr();

// manipulate the data directly within the buffer
*write_ptr = input_value;

// indicate that the write procedure is done and no further write access is performed until an new write pointer is retrieved
advanced_buffer.indicateWriteDone();

/* read access by thread B */

// retrieve read access pointer to newest element
const int* const read_ptr = advanced_buffer.getNewReadAccessPtr(has_new_data);

// read newest value
const int newest_value =  *read_ptr;

// read_ptr remains valid until the next call to getNewReadAccessPtr() or any pop operation
```
While this API is more error prone and requires the use of raw pointers it is especially usefull when storing large objects within the buffer.

Further examples for using the API and a multithread setup can be found in the unit tests located in the test folder. 

## Installation
The only required file to use the buffer is circular_lifo_buffer.h, as it is implemented as header-only class.
For further details you can refer to the Doxygen documentation. The usage can also be seen in the unit tests. 
Besides using the header file, you can integrate the package into a [catkin](http://wiki.ros.org/catkin) workspace.
The implementation has no ROS dependencies.
For building the unit tests you can use `catkin test circular_lifo_buffer`.

## Verification Method
Besides the implemented unit tests, the concept of the buffer was modeled in PROMELA and verified using [spin](https://spinroot.com/spin/whatispin.html).
You can find the model under verification/buffer_verification.pml.

## Future Development & Contribution
The project during which the package was developed has been discontinued.
But in case you find bugs, typos or have suggestions for improvements feel free to open an issue.
We would especially appreciate Pull Requests fixing open issues.

## Author
- Felix Biemüller
