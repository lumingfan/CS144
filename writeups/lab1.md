# Lab 1: Stitching substrings into a Byte Stream


## Preliminaries

> Fill in your name and email address.

J.K. Xu <2507550027@qq.com>

> preliminary comments on your submission

None

> Please cite any offline or online sources you consulted while preparing your submission, other than the lab handouts, course text, lecture notes

None


## Putting substrings in sequence

#### Data Structures

> Copy here the declaration of each new or changed `struct` or `struct member`, `global` or `static variable`, `typedef`, or `enumeration`.
> Identify the purpose of each in 25 words or less.

Added to class StreamReassembler:

```c
size_t _next_data_idx; // the index of next byte which should be pushed into stream
size_t _unass_bytes = 0;  // the number of unassembled bytes (stored in _buffer)
bool _eof = false; // indicator of eof

/**
 * buffer for storing <flag, unassembled byte> pair
 * flag: indicates if this position has a valid byte
 * 
 * invariant:
 * _buffer.size() == capacity - _output.buffer_size()
 */
std::deque<std::pair<bool, char>> _buffer;
```


#### Algorithm

> How to ensure the StreamReassemble maintains its capacity unchanged?

To ensure the capacity remains unchanged, I maintain this invariant: $$\_buffer.size() + \_output.buffer\_size() == \_capacity$$ this invariant can hold true if :
1. Every time we read from _output, we extend the _buffer by the number of bytes we read
2. Every time we extract bytes from the _buffer, we push them to _output (obviously)

> How to extract valid data from the given input (i.e., the portion of data ranging from `first unassembled` to `first unaccepted`)? 

I implemented a function called `truncate()` to extract the valid data. The prototype is as follows:
```c++
/** Extract the valid data which fit in unassembled data range 
 *  @param begin_idx the idx of first unassembled byte
 *  @param end_idx the idx of last unassembled byte
 *  @param idx the index of input data's first byte 
 *  @param data the input data
 *  @return If a valid part exists in the extracted data, 
 *              both the valid part and the idx of its first byte will be returned; 
 *          otherwise, std::nullopt will be returned. 
 */
std::optional<std::pair<size_t, std::string>> 
StreamReassembler::truncate(size_t begin_idx, size_t end_idx, size_t idx, const std::string &data); 
```

it first determines the appropriate idx range of this input data, if no valid range is found, return `std::nullopt`. The proper idx range(idx in the stream) is then converted to idx in this `data`, allowing the use of `string::substr()` to obtain the valid part  


> Briefly describe your implementation of push_string

The implementation of `push_string` is decomposed into several helper functions: 

1. empty_data_handler(): handle empty data input case
```c++
/** check if input data is empty, and if end_input needs to be called
 *  @param data input data 
 *  @param eof the eof flag set by push_string's caller
 */
bool empty_data_handler(const string &data, const bool eof); 
```


2. truncate(): then use truncate() to extract valid data

3. extend_buffer(): extend_buffer() is called to ensure invariance


4. put_data_to_buffer(): my implementation will first store valid bytes in _buffer, then determine whether it is necessary to transfer data from the buffer to the output by inspecting the _buffer().front().flag   

```c++
/** Put the valid bytes to _buffer at given idx
 *  @param idx the idx of the valid bytes' first byte
 *  @param data the valid bytes
 */
void put_data_to_buffer(size_t idx, std::string data);
```

5. push_data_from_buffer(): 

```c++
/** Push the bytes stored in _buffer to _output
 */
void push_data_from_buffer();
```

6. set_eof(): finally, we verify whether we need to set _eof flag and call _output.end_input()

```c++
/** set _eof if eof(param) flag is set and entire data has been received (in either _output or _buffer), 
 *  if _eof is set and the buffer is empty (no unassembled data), we can end_input()
 *  @param eof the eof indicator set by the caller of push_string 
 *  @param last_ch_accepted the last byte in the input has been accepted
 */
void set_eof(bool eof, bool last_ch_accepted);
```


#### Rationale 

> Critique your design, pointing out advantages and disadvantages in
> your design choices.

My implementation use `std::deque` as the unassembled bytes buffer for 
1. fast insertion/deletion at its beginning/end (push data from buffer to output, extend buffer)
2. and random access (store unassembled bytes in the corresponding location) 

Both insertion/deletion and random access operations are O(1). Although deque's random access requires two pointer dereferences, making it less efficient than vector, it outperforms vector in this lab due to the need for efficient insertion/deletion at the front. 

The most serious efficiency issue is that my approach first puts bytes in a buffer before pushing them to output, which doubles the time consumed, however I found it is easier to comprehend and code. 

