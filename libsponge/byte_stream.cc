#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _input_ended(false), _wbytes(0), _rbytes(0), _capacity(capacity), _stream() {}

size_t ByteStream::write(const string &data) {
    size_t nw = 0;
    for (const auto &ch : data) {
        if (remaining_capacity() != 0) {
            _stream.push_back(ch);
            ++nw;
        } else {
            break;
        }
    }
    _wbytes += nw;
    return nw;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    if (len > buffer_size()) {
        throw std::out_of_range("the buffer size is less than request bytes");
    }
    return {_stream.begin(), _stream.begin() + len};
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (len > buffer_size()) {
        set_error();
        throw std::out_of_range("the buffer size is less than request bytes");
    }
    _rbytes += len;
    for (size_t i = 0; i < len; ++i) {
        _stream.pop_front();
    }
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

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _stream.size(); }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _wbytes; }

size_t ByteStream::bytes_read() const { return _rbytes; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
