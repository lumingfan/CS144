#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) { 
    if (!connect_process(seg)) {
        return ;
    } 

    // push data to StreamReassembler    
    size_t idx = seqno_to_idx(extract_seq_no(seg));
    std::string payload = seg.payload().copy();
    _reassembler.push_substring(payload, idx, seg.header().fin);

    // update meta data
    update_meta();
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn) {
        return {};
    }
    return wrap(_ack_no, *_isn);
}

size_t TCPReceiver::window_size() const { 
    return _capacity - stream_out().buffer_size(); 
}

bool TCPReceiver::connect_process(const TCPSegment &seg) {
    // no syn, refused
    if (!_syn && !seg.header().syn) {
        return false;
    }

    // first handshake, syn
    if (!_syn && seg.header().syn) {
        _syn = true;
        _isn = std::make_unique<WrappingInt32>(seg.header().seqno);
        _ack_no = 1;
    }
    return true;
}


WrappingInt32 TCPReceiver::extract_seq_no(const TCPSegment &seg) {
    WrappingInt32 seq_no = seg.header().seqno;
    if (seg.header().syn) {
        seq_no = seq_no + 1;
    }
    return seq_no;
}


size_t TCPReceiver::seqno_to_idx(WrappingInt32 seq_no) { 
    size_t abs_seq_no = unwrap(seq_no, *_isn, _checkpoint);
    return abs_seq_to_stream_idx(abs_seq_no);
}

void TCPReceiver::update_meta() {
    size_t wbytes = stream_out().bytes_written();
    if (wbytes != _checkpoint) {
        _ack_no += wbytes - _checkpoint;
        _checkpoint = wbytes;
    }

    // input ended, if fin has been received, stop increasing _ack_no
    if (stream_out().input_ended() && !_fin) {
        _ack_no += 1;
        _fin = true;
    }
}