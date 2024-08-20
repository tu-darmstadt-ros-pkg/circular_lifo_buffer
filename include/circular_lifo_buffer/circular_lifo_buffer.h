//--------------------------------------------------------------------------------------------------------------------------------
// Copyright 2024 Felix Biemüller, Technische Universität Darmstadt

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED  TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
//--------------------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <atomic>
#include <vector>
#include <assert.h>
#include <functional>

namespace circular_lifo_buffer
{
/**
 * This class implements a circular buffer that behaves as last in first out (LIFO) data structure.
 * It is thread safe for two threads as long as only one thread puts elements into the buffer and only the other thread
 * extracts them. While the class offers a simple interface with hasNewData(), push(T& new_data), pop(T& target_reference) and
 * popIfNew(T& target_reference) also more advanced operations are provided for enabling implementations with more memory
 * efficiency. For these advanced operations the documentation should be read carefully as certain constraints like the
 * the order of the function calls have to be met in order to keep the data consistent and the accesses threadsafe.
 */
template <class T>
class CircularLifoBuffer
{
public:
  CircularLifoBuffer()
  {
    last_written_.store(0, std::memory_order_relaxed);
    current_read_.store(0, std::memory_order_relaxed);
  }

  /**
   * @brief This function can be used to setup all elements of the buffer. It takes a function
   * object which takes a reference of Type T as argument. The given function gets called sequentially with a reference to
   * each element of the buffer.
   * @param element_setup_function This setup function gets called with a reference for each element of the buffer
   * @warning After the instantiation of the Circular Buffer the data refers to uninitialized memory and hence the
   * constructor of the elements might need to be called in the element_setup_function.
   */
  void setupBufferElements(std::function<void(T&)> element_setup_function)
  {
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
      element_setup_function(buffer_[i]);
    }
  }

  /**
   * @brief This function can be used to query whether data was put inside the buffer since the last
   * extraction
   * @return true if data has been put inside
   */
  bool hasNewData() const { return current_read_.load(std::memory_order_seq_cst) != last_written_.load(std::memory_order_seq_cst); }

  /**
   * @brief Puts a new object of type T into the buffer
   * @param new_data The data to be put inside.
   */
  void push(T& new_data)
  {
    T* const write_location = getWriteAccessPtr();
    *write_location = new_data;
    indicateWriteDone();
  }

  /**
   * @brief Extracts an element of the buffer in case a new element was put inside it since the last
   * extraction.
   * @param target_reference reference to which the element type T should be written to. If no new element have been put
   * inside the buffer the it is not overwritten.
   * @return true if a new element was put inside since the last extraction and thus has been extracted
   */
  bool popIfNew(T& target_reference)
  {
    bool has_new_data;
    const T* read_location = getNewReadAccessPtr(has_new_data);
    if (has_new_data)
    {
      target_reference = *read_location;
    }
    return has_new_data;
  }

  /**
   * @brief Extracts the element of the buffer that was written the most recent, no matter whether it has been read
   * allready.
   * @warning If the buffer elements were not initialized with setupBufferElements() the data extracted by
   * this method is uninitialized until the first element was inserted.
   * @param target_reference reference to where the element of type T should be written to.
   *  The target of this reference is overwritten in anycase, even if no element was inserted in the buffer yet.
   * @return true if a new element was written since the last extraction
   */
  bool pop(T& target_reference)
  {
    bool has_new_data;
    const T* read_location = getNewReadAccessPtr(has_new_data);

    target_reference = *read_location;

    return has_new_data;
  }

  /**
   * @brief Returns a pointer to one element of the buffer that is neither the last one written nor
   * read at the moment and thus is safe to be overwritten. After the call to this method the element can be modified.
   * When the modifications are completed and the element should be marked as the next one to be read the function
   * indicateWriteDone() has to be called.
   * @warning indicateWriteDone() should be called exactly one time before the next call to getWriteAccessPtr()
   * happens and no modifications to the data should be done afterwards.
   * @return pointer of type T to one element inside the buffer that can be overwritten
   */
  T* const getWriteAccessPtr()
  {
    assert(!write_in_progress_);

    write_in_progress_ = true;
    setNextWritePosition();
    return &buffer_[next_write_position_];
  }
  /**
   * @brief Indicates that new data was written to the location that was retrieved by the last call of
   * getWriteAccessPtr() and should now be made available for read operations.
   * @warning  IndicateWriteDone>() should be called exactly one time before the next call to
   * getWriteAccessPtr() happens and no modifications to the data should be done afterwards.
   */
  void indicateWriteDone()
  {
    assert(write_in_progress_);
    last_written_.store(next_write_position_, std::memory_order_seq_cst);
    write_in_progress_ = false;
  }

  /**
   * @brief Returns a pointer to the most recent element inside the buffer that can be read safely. The
   * element is as long save to be read until the next extraction is performed eg. by  getNewReadAccessPtr(),
   * getNewReadAccessPtr(bool& has_new_data), pop(T& target_reference) or popIfNew(T& target_reference).
   * @warning If the buffer elements were not initialized with setupBufferElements() the pointer refers to an uninitialized
   * element until the first element was inserted.
   * @return pointer to the element of type T that is the most recent that can be read safely
   */
  T* const getNewReadAccessPtr()
  {
    bool has_new_data;
    return getAndSetCurrentReadPosition(has_new_data);
  }

  /**
   * @brief Returns a pointer to the most recent element inside the buffer that can be read safely. The
   * element is as long save to be read until the next extraction is performed eg. by  getNewReadAccessPtr(),
   * getNewReadAccessPtr(bool& has_new_data), pop(T& target_reference) or popIfNew(T& target_reference).
   * @param has_new_data The reference is set to true, if a insert operation has been performed since the
   * last extraction and else it is set to false.
   * @warning If the buffer elements were not initialized with setupBufferElements() the pointer refers to an uninitialized
   * element until the first element was inserted.
   * @return pointer to the most recently written element of type T that can be read safely
   */
  T* const getNewReadAccessPtr(bool& has_new_data) { return getAndSetCurrentReadPosition(has_new_data); }

  /**
   * @brief Returns the read access pointer that has been set by the last call of pop() or getNewReadAccessPtr().
   * @warning If this function is used the pointer will only point to valid data until the next call of pop() or
   * getNewReadAccessPtr(). Thus this should only be used if it is ensured, that those functions are not called, while
   * the data is still used or it is checked afterwards if these functions have been called. It is NOT sufficient to
   * call getLastSetReadAccessPtr() again and checking if the pointer value has changed, as there still might have been
   * a write call to this position in the meantime again. As this function has to retrive the return value by an atomic
   * variable, it is more efficient to store the pointer retrived by getNewReadAccessPtr() instead of using this method.
   * @return the last read access pointer that has been set
   */
  T* const getLastSetReadAccessPtr() { return &buffer_[current_read_.load(std::memory_order_relaxed)]; }

private:
  static const uint8_t BUFFER_SIZE = 3;

  T buffer_[BUFFER_SIZE];
  std::atomic<uint8_t> last_written_;
  std::atomic<uint8_t> current_read_;

  uint8_t next_write_position_ = 0;

  void setNextWritePosition()
  {
    int current_read_val;
    int current_write_val;
    do
    {
      next_write_position_ = (next_write_position_ + 1) % BUFFER_SIZE;
      current_read_val = current_read_.load(std::memory_order_seq_cst);
      current_write_val = last_written_.load(std::memory_order_seq_cst);
    } while (!(next_write_position_ != current_read_val && next_write_position_ != current_write_val));
    assert(next_write_position_ >= 0 && next_write_position_ < BUFFER_SIZE);
  }

  T* const getAndSetCurrentReadPosition(bool& is_new_position)
  {
    uint8_t last_written_ptr;
    uint8_t old_read_pointer;

    /* The following while loop is critical to ensure that last_written has not changed between the load and exchange
     * operation otherwise there is a point in time, where last written could have been already changed and current_read
     * has not changed yet to its new value, so SetNextWritePosition() could select the position which is about to be
     * stored in current_read.
     */
    /* In theory this could lead to read starvation, but as the writer has to write the buffer entry in the mean time
     * and generate new data to be written, this should be no problem in a practical application.
     */
    do
    {
      last_written_ptr = last_written_.load(std::memory_order_seq_cst);
      old_read_pointer = current_read_.exchange(last_written_ptr, std::memory_order_seq_cst);
    } while (last_written_.load(std::memory_order_seq_cst) != last_written_ptr);

    is_new_position = old_read_pointer != last_written_ptr;
    return &buffer_[last_written_ptr];
  }

  bool write_in_progress_ = false;
};
}  // namespace circular_lifo_buffer
