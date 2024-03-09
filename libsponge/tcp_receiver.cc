#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) { 
    // no syn, refused
    if (!_syn && !seg.header().syn) {
        return ;
    }

    // first handshake, syn
    if (!_syn && seg.header().syn) {
        _syn = true;
        _isn = std::make_unique<WrappingInt32>(seg.header().seqno);
        _ack_no = 1;
    }

    // push data to StreamReassembler    
    WrappingInt32 seq_no = seg.header().seqno;
    if (seg.header().syn) {
        seq_no = seq_no + 1;
    }
    size_t abs_seq_no = unwrap(seq_no, *_isn, _checkpoint);
    size_t idx = abs_seq_to_stream_idx(abs_seq_no);
    std::string payload = seg.payload().copy();
    _reassembler.push_substring(payload, idx, seg.header().fin);

    // update meta data
    size_t wbytes = stream_out().bytes_written();
    if (wbytes != _checkpoint) {
        _ack_no += wbytes - _checkpoint;
        _checkpoint = wbytes;
    }
    if (stream_out().input_ended()) {
        _ack_no += 1;
    }
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
