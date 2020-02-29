/*
 *
 * ChatBoxServer.h
 *
 */

#ifndef CHATBOXSERVER_H
#define CHATBOXSERVER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>

#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include "protocol.h"

typedef std::array<char, MAX_PACK_SIZE> customArray;
typedef std::deque<customArray> dequeOfArrays;

using boost::asio::ip::tcp;

namespace chat_box_server {

class ChatParticipant
{
public:
    virtual ~ChatParticipant() {}
    virtual void onMessage(customArray &msg) = 0;
};

typedef std::shared_ptr<ChatParticipant> chatParticipantPtr;

class ChatRoom
{
public:
    void joinRoom(chatParticipantPtr participant, const std::string &nickname);
    void leaveRoom(chatParticipantPtr participant);
    void broadcast(const customArray& msg, chatParticipantPtr participant);
    std::string getNickname(chatParticipantPtr participant);
private:
    std::unordered_set<chatParticipantPtr> participants;
    std::unordered_map<chatParticipantPtr, std::string> participantTable;
    enum { max_recent_msgs = 100 };
    dequeOfArrays recent_msgs;
};

class ChatSession : public ChatParticipant, public std::enable_shared_from_this<ChatSession>
{
public:
    ChatSession(boost::asio::io_context &io_context,
                boost::asio::io_context::strand &strand, ChatRoom &room);
    tcp::socket& getSocket();
    void start();
    void onMessage(customArray& msg);
    void readHandler(const boost::system::error_code& error);
    void writeHandler(const boost::system::error_code& error);
    void nicknameHandler(const boost::system::error_code& error);
private:
    tcp::socket mSocket;
    boost::asio::io_context::strand &mStrand;
    ChatRoom &mRoom;
    customArray mNickname;
    customArray mReadMsg;
    dequeOfArrays mWriteMsgs;
};
typedef std::shared_ptr<ChatSession> chatSessionPtr;

class ChatServer
{
public:
    ChatServer(boost::asio::io_context &io_context,
               boost::asio::io_context::strand &strand,
               const tcp::endpoint &endpoint);
    void startAccept();
    void handleAccept(chatSessionPtr session,
                      const boost::system::error_code& error);
private:
    boost::asio::io_context &mIo_context;
    boost::asio::io_context::strand &mStrand;
    tcp::acceptor mAcceptor;
    ChatRoom mRoom;
};

} // chat_box_server

#endif // !CHATBOXSERVER_H
