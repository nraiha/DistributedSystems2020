/*
 *
 * ChatBox.h
 *
 */

#ifndef CHAT_BOX_CLIENT_H
#define CHAT_BOX_CLIENT_H

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <deque>
#include "protocol.h"

using boost::asio::ip::tcp;

typedef std::array<char, MAX_PACK_SIZE> customArray;
typedef std::deque<customArray> dequeofArrays ;

class ChatBox
{
    public:
    ChatBox(std::array<char, MAX_NICKNAME_SIZE>& nickname,
                boost::asio::io_context& io_context,
                tcp::resolver::iterator endpoint_iterator);
    ~ChatBox(){}
    void write(const customArray& msg);
    void close();
    private:
    void handleConnect(const boost::system::error_code& error);
    void handleRead(const boost::system::error_code& error);
    void doWrite(customArray msg);
    void handleWrite(const boost::system::error_code& error);
    void doClose();
    void reconnect();

    private:
    boost::asio::io_context& mIo_context;
    tcp::socket mSocket;
    tcp::resolver::iterator eIterator;
    customArray mReadMsgs;
    dequeofArrays mWriteMsgs;
    customArray mClientName;
};

#endif /* !CHAT_BOX_CLIENT_H */
