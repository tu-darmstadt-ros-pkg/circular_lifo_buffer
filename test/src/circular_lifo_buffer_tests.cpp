#include <gtest/gtest.h>

#include <chrono>
#include <unistd.h>
#include <thread>

#include "circular_lifo_buffer/circular_lifo_buffer.h"

namespace circular_lifo_buffer
{
namespace test
{
TEST(BasicBuffer, SingleInsertAndExtract)
{
  CircularLifoBuffer<int> basic_buffer;
  bool has_new_data;
  int input_value = 4;

  int ret = 7;
  /* no new data should be there after initialization */
  has_new_data = basic_buffer.hasNewData();
  EXPECT_EQ(has_new_data, false) << "Indicates new data after initialization when using HasNewData";

  /* assert that ret is not changed, if no new data is available */
  has_new_data = basic_buffer.popIfNew(ret);
  EXPECT_EQ(has_new_data, false) << "Indicates new data after initialization when using TryPop";
  EXPECT_EQ(ret, 7) << "Sets returnvale even if no new data available";

  basic_buffer.push(input_value);
  /* assert original data has not been modified */
  EXPECT_EQ(input_value, 4) << "Manipulates input data";

  /* now new data should be tavailable */
  has_new_data = basic_buffer.hasNewData();
  EXPECT_EQ(has_new_data, true) << "Indicates no new data even if there is some  when using HasNewData";

  /* assert the correct result after a pop */
  has_new_data = basic_buffer.popIfNew(ret);
  EXPECT_EQ(ret, 4) << "Extracts wrong value";
  EXPECT_EQ(has_new_data, true) << "Indicates no new data even if there is some  when using TryPop";

  /* now no new data should be tavailable anymore */
  has_new_data = basic_buffer.hasNewData();
  EXPECT_EQ(has_new_data, false) << "Still indicates new data after extract when using HasNewData";

  /* assert the correct result after a pop */
  has_new_data = basic_buffer.popIfNew(ret);
  EXPECT_EQ(ret, 4) << "Extracts values after there shouldn't be anymore";
  EXPECT_EQ(has_new_data, false) << "Indicates new data after extraction when using TryPop";
}

TEST(BasicBuffer, MultipleInsertAndExtract)
{
  CircularLifoBuffer<int> basic_buffer;
  bool has_new_data;

  int input_values[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  int output_values[] = { 2, 5, 8, 9 };

  int output_counter = 0;
  for (int i = 0; i < 9; i++)
  {
    int input_value = input_values[i];
    basic_buffer.push(input_value);
    has_new_data = basic_buffer.hasNewData();

    /* There should always be data after insertion available */
    EXPECT_EQ(has_new_data, true) << "Indicates no new data even if there is some, after pushing " << input_value;

    if (input_value == output_values[output_counter])
    {
      int ret;
      has_new_data = basic_buffer.popIfNew(ret);
      EXPECT_EQ(has_new_data, true) << "Indicates no new data even if there is some  when using TryPop, after pushing " << input_value;
      EXPECT_EQ(ret, output_values[output_counter]) << "Extracts wrong value after pushing " << input_value;

      has_new_data = basic_buffer.hasNewData();
      EXPECT_EQ(has_new_data, false) << "Still indicates new data after extract when using HasNewData, after pushing " << input_value;
      output_counter++;
    }
  }
}

TEST(BasicBuffer, InitializeBuffer)
{
  CircularLifoBuffer<int> basic_buffer;

  int zero = 0;
  basic_buffer.push(zero);
  basic_buffer.setupBufferElements([](int& element) { element = 3; });

  int result = -1;
  basic_buffer.pop(result);

  EXPECT_EQ(result, 3) << "Buffer entries were not initialized correctly";
  basic_buffer.push(zero);
  basic_buffer.push(zero);
  basic_buffer.setupBufferElements([](int& element) { element = 7; });
  basic_buffer.pop(result);
  EXPECT_EQ(result, 7) << "Buffer entries were not initialized correctly";
}

TEST(AdvancedBuffer, MultipleInsertAndExtract)
{
  CircularLifoBuffer<int> advanced_buffer;
  bool has_new_data;

  std::vector<int> input_values = { 9, 8, 7, 6, 5, 4, 3, 2, 1 };
  std::vector<int> output_values = { 9, 8, 4, 1 };

  int output_counter = 0;
  for (const int& input_value : input_values)
  {
    // write data via pointer
    int* const write_ptr = advanced_buffer.getWriteAccessPtr();
    *write_ptr = input_value;
    advanced_buffer.indicateWriteDone();

    has_new_data = advanced_buffer.hasNewData();

    /* There should always be data after insertion available */

    ASSERT_EQ(has_new_data, true) << "Indicates no new data even if there is some, after pushing " << input_value;

    if (input_value == output_values[output_counter])
    {
      int ret;
      const int* const read_ptr = advanced_buffer.getNewReadAccessPtr(has_new_data);

      EXPECT_EQ(has_new_data, true) << "Indicates no new data even if there is some  when using TryPop, after pushing " << input_value;

      ret = *read_ptr;
      EXPECT_EQ(ret, output_values[output_counter]) << "Extracts wrong value after pushing " << input_value;

      has_new_data = advanced_buffer.hasNewData();
      EXPECT_EQ(has_new_data, false) << "Still indicates new data after extract when using HasNewData, after pushing " << input_value;
      output_counter++;
    }
  }
}

/* Beginning of helper functions for multithread test */

long getTimeInMs()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void insert(CircularLifoBuffer<int>* buffer, int* first_element, int element_nr, int write_delay)
{
  struct timespec sleep_time;
  sleep_time.tv_sec = 0;      // seconds
  sleep_time.tv_nsec = 1000;  // nanoseconds
  nanosleep(&sleep_time, NULL);

  for (int i = 0; i < element_nr; i++)
  {
    int* access_ptr = buffer->getWriteAccessPtr();
    *access_ptr = first_element[i];
    buffer->indicateWriteDone();
  }
}

void extract(CircularLifoBuffer<int>* buffer, int* first_element, int element_nr, int* successful_reads, int read_delay, int timeout)
{
  int counter = 0;
  int old_counter = 0;
  long start_time = getTimeInMs();
  int read_value = -1;
  *successful_reads = 0;

  while (getTimeInMs() - start_time < timeout)
  {
    // wait until buffer has gotten a new value
    bool has_new_data;
    const int* access_ptr = buffer->getNewReadAccessPtr(has_new_data);
    int activate_sleep_counter = 0;
    while (!has_new_data)
    {
      access_ptr = buffer->getNewReadAccessPtr(has_new_data);
      activate_sleep_counter++;
      if (activate_sleep_counter > 1000)
      {
        // sleep for 1 nano second to ensure that another thread can start to insert values
        //  without this the unit tests on the CI server failed from time to time
        struct timespec sleep_time;
        sleep_time.tv_sec = 0;     // seconds
        sleep_time.tv_nsec = 100;  // nanoseconds
        nanosleep(&sleep_time, NULL);
      }
      if (getTimeInMs() - start_time > timeout)
      {
        ASSERT_TRUE(false) << "Could not read any values written by other thread within timeout";
      }
    };
    read_value = *access_ptr;
    *successful_reads = *successful_reads + 1;
    old_counter = counter;
    while (first_element[counter] != read_value)
    {
      counter++;
      if (counter >= element_nr)
      {
        EXPECT_TRUE(false) << "Element was read, that was not put into the buffer or in the wrong order. Read Element: " << read_value << " Counter: " << counter
                           << " Old Counter: " << old_counter;
        return;
      }
    }
    assert(read_value == counter);
    if (counter == element_nr - 1)
    {
      EXPECT_EQ(read_value, first_element[element_nr - 1]) << "The last read element has an incorrect value";
      return;  // the last element was extracted
    }
  }
  ASSERT_TRUE(false) << "The maximum time allowed until the last element should be read was reached. Last value read: " << read_value << " Counter: " << counter
                     << " Old Counter: " << old_counter;
}

/* Ending  of helper functions for multithread test */

TEST(AdvancedBuffer, MultiThreadedTest)
{
  int nr_of_values = 100000;
  int* input_values = new int[nr_of_values];

  for (int i = 0; i < nr_of_values; i++)
  {
    input_values[i] = i;
  }
  double avg_success_rate = 0;
  int test_cycles = 20;
  for (int i = 0; i < test_cycles; i++)
  {
    CircularLifoBuffer<int> advanced_buffer;
    int successful_reads = 0;

    std::thread writer(insert, &advanced_buffer, input_values, nr_of_values, 0);

    std::thread reader(extract, &advanced_buffer, input_values, nr_of_values, &successful_reads, 0, 10000);

    writer.join();
    reader.join();
    double success_rate = successful_reads * 1.0 / nr_of_values * 100;
    avg_success_rate += success_rate;
  }
  avg_success_rate = avg_success_rate / test_cycles;
  std::cout << "[          ] [ INFO ] "
            << "Successfully extracted an average of " << avg_success_rate << "% of the values.\n";
}
}  // namespace test
}  // namespace circular_lifo_buffer
