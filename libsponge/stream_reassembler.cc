#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _next_data_idx(0), _buffer(){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // empty data handler
    if (empty_data_handler(data, eof)) {
        return ;
    }

    const auto &pair_opt = 
        truncate(begin_idx(), end_idx(), index, data);
    
    // this data is invalid 
    // 1. the entire data has been pushed into _output before
    // 2. index is at the first unacceptable index or beyond
    if (pair_opt == std::nullopt) {
        return ;
    }

    const auto &[new_idx, new_data] = pair_opt.value();
    extend_buffer(available_capacity());
    put_data_to_buffer(new_idx, new_data);
    push_data_from_buffer();

    bool last_ch_accepted = 
        (index + data.length() == new_idx + new_data.length());
    set_eof(eof, last_ch_accepted);
}

size_t StreamReassembler::unassembled_bytes() const { return _unass_bytes; }

bool StreamReassembler::empty() const {
    return unassembled_bytes() == 0;
}

std::optional<std::pair<size_t, std::string>> StreamReassembler::truncate(size_t begin_idx, size_t end_idx, size_t idx, const std::string &data) {
    size_t data_end_idx = data.length() + idx - 1;
    if (begin_idx > data_end_idx) {
        return std::nullopt;
    } 
    if (end_idx < idx) {
        return std::nullopt;
    }
    size_t pos = begin_idx > idx ? begin_idx : idx;
    size_t end_pos = end_idx < data_end_idx? end_idx : data_end_idx;
    size_t npos = end_pos - pos + 1;

    size_t trc_begin_idx = pos - idx;
    std::string truncated_data = data.substr(trc_begin_idx, npos);
    std::pair<size_t, std::string> res{pos, truncated_data};
    return {res};
}

void StreamReassembler::put_data_to_buffer(size_t idx, std::string data) {
    size_t actual_idx = idx - _next_data_idx;
    for (size_t i = 0; i < data.length(); ++i) {
        if (!_buffer[i + actual_idx].first) {
            ++_unass_bytes; 
        }
        _buffer[i + actual_idx] = std::make_pair(true, data[i]);
    }
}

void StreamReassembler::push_data_from_buffer() {
    std::string str;
    while (!_buffer.empty() && _buffer.front().first) {
        str.push_back(_buffer.front().second); 
        _buffer.pop_front();
        --_unass_bytes;
    }
    _next_data_idx += str.length();
    _output.write(str);
}
void StreamReassembler::extend_buffer(size_t new_size) {
    while (_buffer.size() < new_size) {
        _buffer.push_back({false, '\0'});
    }
}
    
bool StreamReassembler::empty_data_handler(const string &data, const bool eof) {
    if (data.empty()) {
        if (eof) {
            _output.end_input();
        }
        return true;
    }
    return false;
}

void StreamReassembler::set_eof(bool eof, bool last_ch_accepted) {
    // set _eof if the entire data has been _buffer or _output
    if (eof && last_ch_accepted){
        _eof = true;
    }

    // end input when all data in _buffer have been pushed into _output
    if (_eof && empty()) {
        _output.end_input();
    }
}