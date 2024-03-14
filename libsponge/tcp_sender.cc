#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _remained_time_to_timeout(retx_timeout)
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // normal segment with payloads
    while (available_window() != 0) {

        std::string payload = _stream.read(max_payload_size());
        _checkpoint = _stream.bytes_read();
        uint64_t size = payload.size();

        bool fin_flag = false;
        bool syn_flag = false;
        if (fin_need_to_be_sent(size)) {
            fin_flag = true;
        }

        if (syn_need_to_be_sent()) {
            // syn segment, only be sent once
            syn_flag = true;
        }

        // no message should be sent
        if (size == 0 && !fin_flag && !syn_flag) {
            break ;
        }

        TCPSegment seg = create_tcp_segment(syn_flag, fin_flag, wrap(_next_seqno, _isn), payload);
        send_tcp_segment(seg, size);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    _window_size = window_size;

    // ignore invalid ackno 
    if (!check_ackno_valid(ackno)) {
        return ;
    }

    // remove acked segments
    while (!_segments_outstanding.empty() && check_seqno_acked(_segments_outstanding.front(), ackno)) {
        _segments_outstanding.pop_front();
    }

    // Set the RTO back to its “initial value.”
    _retransmission_timeout = _initial_retransmission_timeout;
    _retrans_times = 0;

    // restart timer
    restart_timer();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if (ms_since_last_tick < _remained_time_to_timeout) {
        _remained_time_to_timeout -= ms_since_last_tick;
        return ;
    }

    // timeout
    if (!_segments_outstanding.empty()) {
        _segments_out.push(_segments_outstanding.front());
    }

    if (_window_size != 0) {
        _retrans_times += 1;
        _retransmission_timeout <<= 1;
    }

    restart_timer();
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retrans_times; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(TCPSegment());
}

uint64_t TCPSender::max_payload_size() const { 
    uint64_t res = available_payload_size();
    res = res < TCPConfig::MAX_PAYLOAD_SIZE ? res : TCPConfig::MAX_PAYLOAD_SIZE;
    return res;
}

uint64_t TCPSender::available_payload_size() const {
    uint64_t buffer_size = _stream.buffer_size();
    uint64_t available_window_size = available_window();
    uint64_t res = available_window_size < buffer_size ? available_window_size : buffer_size; 
    return res;
}

TCPSegment TCPSender::create_tcp_segment(bool syn, bool fin, WrappingInt32 seq_no, std::string &payload) const {
    TCPSegment seg;
    seg.header().syn = syn;
    seg.header().fin = fin;
    seg.header().seqno = seq_no;
    seg.payload() = Buffer(std::move(payload));
    return seg;
}

void TCPSender::send_tcp_segment(TCPSegment seg, uint64_t n) {
    if (seg.header().syn) {
        n += 1;
    }
    if (seg.header().fin) {
        n += 1;
    }
    increase_next_seqno(n);
    _bytes_in_flight += n;

    // backup TCP segment
    _segments_outstanding.push_back(seg);
    // sent syn message
    _segments_out.push(seg);
}

bool TCPSender::check_seqno_acked(const TCPSegment &seg, const WrappingInt32 ackno) { 
    uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, _checkpoint);
    uint64_t payload_length = seg.payload().size();
    uint64_t abs_ackno = unwrap(ackno, _isn, _checkpoint);

    if (seg.header().syn) {
        payload_length += 1;
    }
    if (seg.header().fin) {
        payload_length += 1;
    }

    bool res = abs_seqno + payload_length <= abs_ackno;

    if (res) {
        _bytes_in_flight -= payload_length;
    }
    return res;
}

bool TCPSender::check_ackno_valid(const WrappingInt32 ackno) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _checkpoint);
    // ackno is beyond next seqno
    if (abs_ackno > _next_seqno) {
        return false;
    }

    // outdated ackno 
    if (abs_ackno <= _last_ackno) {
        return false;
    }

    _last_ackno = abs_ackno;
    return true;
}
