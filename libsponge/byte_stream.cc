#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : input_ended_(false)
    , stream_(capacity)
    , wcurr_(0)
    , rcurr_(0)
    , buffer_size_(0)
    , wbytes_(0)
    , rbytes_(0)
    , capacity_(capacity) {  // DUMMY_CODE(capacity);
}

size_t ByteStream::write(const string &data) {
    size_t nw = 0;
    for (const auto &ch : data) {
        if (remaining_capacity() != 0) {
            stream_[wcurr_] = ch;
            advanceWcurr(1);
            ++buffer_size_;
            ++nw;
        } else {
            break;
        }
    }

    wbytes_ += nw;
    return nw;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    std::string str;
    for (size_t i = 0; i < len; ++i) {
        str.push_back(stream_[(rcurr_ + i) % capacity_]);
    }
    return str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    advanceRcurr(len);
    rbytes_ += len;
    buffer_size_ -= len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string str;
    for (size_t i = 0; i < len && !eof() && !buffer_empty(); ++i) {
       str += peek_output(1); 
       pop_output(1);
    }
    return str;
}

void ByteStream::end_input() { input_ended_ = true; }

bool ByteStream::input_ended() const { return input_ended_; }

size_t ByteStream::buffer_size() const { return buffer_size_; }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return wbytes_; }

size_t ByteStream::bytes_read() const { return rbytes_; }

size_t ByteStream::remaining_capacity() const { return capacity_ - buffer_size(); }
