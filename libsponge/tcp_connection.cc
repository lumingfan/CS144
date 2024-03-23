#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return {}; }

void TCPConnection::send_segments() {
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        auto ackno = _receiver.ackno();
        auto win_size = _receiver.window_size();
        if (ackno.has_value()) {
            seg.header().ack = true;
            seg.header().ackno = ackno.value();
        }
        seg.header().win = win_size;
        _sender.segments_out().pop();
        _segments_out.push(seg);
    }
}

void TCPConnection::send_rst() {
    _sender.send_empty_segment();
    auto seg = _sender.segments_out().back();
    seg.header().rst = true;
    _segments_out.push(seg);
}

void TCPConnection::abort_connection() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();    
    _active = false;
    return ;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    // keep listen util syn request
    if (!seg.header().syn && _sender.next_seqno_absolute() == 0) {
        return ;
    }

    // look at if RST flag has been set
    if (seg.header().rst) {
        abort_connection();
        return ;
    }

    // pass the segment to the TCPReceiver
    _receiver.segment_received(seg);

    // ack is set, tells the TCPSender about the `ackno` and `window_size`
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    // reply
    _sender.fill_window();
    if (seg.header().syn || seg.payload().size() != 0) {
        // at least one segment is sent in reply
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        send_segments();
    }
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t nwritten = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segments();
    return nwritten;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);

    // abort the connection 
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        abort_connection();
        send_rst();
    } else {
        send_segments(); 
    }
}

void TCPConnection::end_input_stream() {_sender.stream_in().end_input();}

void TCPConnection::connect() {
    _sender.fill_window();
    send_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
