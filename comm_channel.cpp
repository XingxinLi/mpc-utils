#include "comm_channel.hpp"
#include "absl/memory/memory.h"

// constructor called from party::connect_to
comm_channel::comm_channel(std::unique_ptr<boost::asio::ip::tcp::iostream>&& s,
                           party& p, int peer_id, bool measure_communication)
    : p(p),
      id(p.get_id()),
      tcp_stream(std::move(s)),
      need_flush(false),
      measure_communication(measure_communication),
      sent_header_size(-1),
      received_header_size(-1) {
  // Build output archive, optionally measuring communication.
  if (!measure_communication) {
    oarchive = absl::make_unique<boost::archive::binary_oarchive>(
        *tcp_stream, boost::archive::no_codecvt);
  } else {
    ostream = absl::make_unique<boost::iostreams::filtering_ostream>();
    ostream->push(boost::iostreams::counter(), 0, 0);
    ostream->push(*tcp_stream, 0, 0);
    oarchive = absl::make_unique<boost::archive::binary_oarchive>(
        *ostream, boost::archive::no_codecvt);
  }

  // Flush so that headers get written to the network that need to be read when
  // the other party constructs their input archive.
  flush();

  // Build input archive, optionally measuring communication.
  if (!measure_communication) {
    iarchive = absl::make_unique<boost::archive::binary_iarchive>(
        *tcp_stream, boost::archive::no_codecvt);
  } else {
    istream = absl::make_unique<boost::iostreams::filtering_istream>();
    istream->push(boost::iostreams::counter(), 0, 0);
    istream->push(*tcp_stream, 0, 0);
    iarchive = absl::make_unique<boost::archive::binary_iarchive>(
        *istream, boost::archive::no_codecvt);
  }

  if (peer_id == -1) {
    recv(peer_id);  // read id of remote
  } else {
    send(this->id);  // send own ID to remote
    flush();
  }

  if (measure_communication) {
    sent_header_size =
        ostream->component<boost::iostreams::counter>(0)->characters();
    received_header_size =
        istream->component<boost::iostreams::counter>(0)->characters();
  }

  if (peer_id < 0 || peer_id == id) {
    BOOST_THROW_EXCEPTION(
        boost::enable_error_info(std::invalid_argument("invalid peer_id"))
        << error_num_servers(p.get_num_servers()) << error_my_id(id)
        << error_peer_id(peer_id));
  }
  this->peer_id = peer_id;
};

// write & read functions for direct binary access
void comm_channel::write(const char* data, size_t size) {
  COMM_CHANNEL_WRAP_EXCEPTION(oarchive->save_binary(data, size),
                              boost::archive::archive_exception);
  need_flush = true;
}
void comm_channel::read(char* buffer, size_t size) {
  flush_if_needed();
  COMM_CHANNEL_WRAP_EXCEPTION(iarchive->load_binary(buffer, size),
                              boost::archive::archive_exception);
}

#ifdef MPC_UTILS_USE_SCAPI
// write & read functions for CommParty interface
void comm_channel::write(const byte* data, int size) {
  write(reinterpret_cast<const char*>(data), size);
}
size_t comm_channel::read(byte* buffer, int size) {
  read(reinterpret_cast<char*>(buffer), size);
  return size;
}
#endif

// create a second comm_channel from this one; establishes a new connection
comm_channel comm_channel::clone() {
  boost::asio::ip::tcp::no_delay no_delay;
  tcp_stream->rdbuf()->get_option(no_delay);
  flush_if_needed();
  return p.connect_to(peer_id, measure_communication, no_delay.value());
}

// flush underlying stream
void comm_channel::flush() {
  if (measure_communication) {
    COMM_CHANNEL_WRAP_EXCEPTION(ostream->strict_sync(), std::ios_base::failure);
  }
  // strict_sync does not seem to flush tcp_stream, so we do it here even if
  // measure_communication is true.
  COMM_CHANNEL_WRAP_EXCEPTION(tcp_stream->flush(), std::ios_base::failure);
  need_flush = false;
}

void comm_channel::sync() {
  int a = 0, b;
  send_recv(a, b);
  // // sequentially send and receive to save the reconnection that would
  // // occur with send_recv
  // if(this->get_id() < this->get_peer_id()) {
  //   send(a);
  //   flush();
  //   recv(b);
  // } else {
  //   recv(b);
  //   send(a);
  //   flush();
  // }
}

int64_t comm_channel::get_num_bytes_sent() {
  if (!measure_communication) {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
        "get_num_bytes_sent may only be called if measure_communication = true "
        "was passed at construction"));
  }
  int64_t sent =
      ostream->component<boost::iostreams::counter>(0)->characters() -
      sent_header_size;
  if (twin) {
    sent += twin->get_num_bytes_sent();
  }
  return sent;
}

int64_t comm_channel::get_num_bytes_received() {
  if (!measure_communication) {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
        "get_num_bytes_received may only be called if measure_communication = "
        "true was passed at construction"));
  }
  int64_t received =
      istream->component<boost::iostreams::counter>(0)->characters() -
      received_header_size;
  if (twin) {
    received += twin->get_num_bytes_received();
  }
  return received;
}
