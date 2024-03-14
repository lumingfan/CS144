#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! outstanding deque of segments that the TCPSender 
    std::deque<TCPSegment> _segments_outstanding{};

    //! initial retransmission timer for the connection, don't change 
    const unsigned int _initial_retransmission_timeout;

    //! retransmission timer for the connection
    unsigned int _retransmission_timeout;
    //! the remained time to trig timeout
    unsigned int _remained_time_to_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    //! the (absolute) acked number for the last ack_received
    uint64_t _last_ackno{0};

    //! the window's size, default is 1 (i.e. haven't receive reply) 
    uint16_t _window_size{1};

    //! checkpoint, numbers of rbytes of stream 
    uint64_t _checkpoint{0};

    //! bytes in flight
    uint64_t _bytes_in_flight{0};

    //! the number of consecutive retransmissions
    uint64_t _retrans_times{0};


  private: 
    //! does the first syn has been sent
    inline bool syn_need_to_be_sent() const {
      return _next_seqno == 0; 
    }

    //! does the fin needs to be sent
    //! if payload_size is equal to available_window_size, 
    //! then fin can't be send, 
    //! because fin also occupy window space by 1 
    inline bool fin_need_to_be_sent(uint64_t payload_size) const {
      return _stream.eof() && _next_seqno < _stream.bytes_written() + 2 && available_window() > payload_size;
    }

    //! get available window size
    inline uint64_t available_window() const {
      if (_window_size == 0) {
        // treat window_size as 1
        if (_bytes_in_flight != 0) {
          return 0;
        }
        return 1;
      }
      if (_window_size <= _bytes_in_flight) {
        return 0;
      }
      return _window_size - _bytes_in_flight;
    } 

    //! get available payload size
    //! i.e. min(available window size, available buffer size)
    uint64_t available_payload_size() const ;

    //! get the available payload size by comparing _window_size, 
    //! buffer_size of _stream and TCPConfig::MAX PAYLOAD SIZE
    //! if window size is 0 and buffer size is not 0, return 1
    uint64_t max_payload_size() const ;




    //! increment _next_seqno by n
    inline void increase_next_seqno(uint64_t n) {
      _next_seqno += n;
    }

    
    //! Create a tcp segment with given syn, fin, seq_no and payload
    //!!!! payload will be invalid after calling this function
    TCPSegment create_tcp_segment(bool syn, bool fin, WrappingInt32 seq_no, std::string &payload) const ;

    //! send tcp segment with a backup, then increase _next_seqno by n  
    void send_tcp_segment(TCPSegment seg, uint64_t n);

    //! check if this seg has been acked
    bool check_seqno_acked(const TCPSegment &seg, const WrappingInt32 ackno) ;

    //! check if this ack is valid
    bool check_ackno_valid(const WrappingInt32 ackno) ;

    //! restart retransmission timer
    inline void restart_timer() {
      _remained_time_to_timeout = _retransmission_timeout;
    }

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
