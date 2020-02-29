/*
 *
 * ChatBoxServer.cpp
 *
 */

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <iomanip>
#include <iostream>
#include <list>
#include <set>
#include <thread>
#include <fstream>
#include <vector>

#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>

#include "protocol.h"
#include "ChatBoxServer.h"

using namespace chat_box_server;
using boost::asio::ip::tcp;

typedef std::array<char, MAX_PACK_SIZE> customArray;
typedef std::deque<customArray> dequeOfArrays;

std::vector<std::string> backup_msg;
std::ifstream in;
std::ofstream out("messages.txt");
int flag=0;

//----------------------------------------------------------------------

std::string getTime()
{
    boost::posix_time::ptime time = boost::posix_time::second_clock::local_time();
    boost::posix_time::time_duration duration = time.time_of_day();

    std::stringstream str;
    str << '[' << duration << "] ";
    return str.str();
}

class WorkerThread
{
public:
    static void run(std::shared_ptr<boost::asio::io_context> io_context)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            LOG(INFO) << "[" << std::this_thread::get_id() << "] Thread starts";
        }
        io_context->run();
        {
          std::lock_guard<std::mutex> lock(mtx);
          LOG(INFO) << "[" << std::this_thread::get_id() << "] Thread ends";
        }
    }

private:
    static std::mutex mtx;
};

std::mutex WorkerThread::mtx;


void ChatRoom::joinRoom(chatParticipantPtr participant, const std::string &nickname)
{
    participants.insert(participant);
    participantTable[participant] = nickname;

    // Get the whole string and convert it to char
    std::string str = "joined the chat";
    char ch[str.size() + 1];
    strcpy(ch, &str[0]);
    // Copy the message from chat to customArray and broadcast it
    customArray message;
    strcpy(message.data(), ch);
    broadcast(message, participant);
    recent_msgs.pop_front();
    std::for_each(recent_msgs.begin(), recent_msgs.end(),
              boost::bind(&ChatParticipant::onMessage, participant, _1));
}

void ChatRoom::leaveRoom(chatParticipantPtr participant)
{
    // Get the whole string and convert it to char
    std::string str = "left the chat";
    char ch[str.size() + 1];
    strcpy(ch, &str[0]);
    // Copy the message from chat to customArray and broadcast it
    customArray message;
    strcpy(message.data(), ch);
    broadcast(message, participant);

    participants.erase(participant);
    participantTable.erase(participant);
}

void ChatRoom::broadcast(const customArray &msg, chatParticipantPtr participant)
{

    if (flag==1)
    {
        for (auto i = backup_msg.begin(); i != backup_msg.end(); ++i) {
            std::string str = *i;
            customArray bMsg;
            strcpy(bMsg.data(), str.c_str());
            recent_msgs.push_back(bMsg);
        }
    flag = 0;
    }

    std::string timeStamp = getTime();
    std::string name = getNickname(participant);
    customArray f_message;

    std::stringstream ss;
    ss << timeStamp << name << msg.data();
    std::string str = ss.str();
    strcpy(f_message.data(), str.c_str());

    out << str.c_str() << std::endl;

    recent_msgs.push_back(f_message);
    while (recent_msgs.size() > max_recent_msgs)
        recent_msgs.pop_front();

    std::for_each(
        participants.begin(), participants.end(),
        boost::bind(&ChatParticipant::onMessage, _1, boost::ref(f_message)));
}

std::string ChatRoom::getNickname(chatParticipantPtr participant)
{
    return participantTable[participant];
}

//----------------------------------------------------------------------

ChatSession::ChatSession(boost::asio::io_context &io_context,
           boost::asio::io_context::strand &strand, ChatRoom &room)
    : mSocket(io_context), mStrand(strand), mRoom(room) {}

tcp::socket& ChatSession::getSocket()
{
    return mSocket;
}

void ChatSession::start()
{
    boost::asio::async_read(
            mSocket, boost::asio::buffer(mNickname, mNickname.size()),
            mStrand.wrap(boost::bind(&ChatSession::nicknameHandler,
                             shared_from_this(), _1)));
}

void ChatSession::onMessage(customArray &msg)
{
    bool write_in_progress = !mWriteMsgs.empty();
    mWriteMsgs.push_back(msg);

    if (!write_in_progress) {
        boost::asio::async_write(
              mSocket,
              boost::asio::buffer(mWriteMsgs.front().data(),
                          mWriteMsgs.front().size()),
              mStrand.wrap(boost::bind(&ChatSession::writeHandler,
                               shared_from_this(), _1)));
    }
}

void ChatSession::readHandler(const boost::system::error_code &error)
{
    LOG(INFO) << "Message received";
    if (!error) {
        mRoom.broadcast(mReadMsg, shared_from_this());
        boost::asio::async_read(
                mSocket, boost::asio::buffer(mReadMsg, mReadMsg.size()),
                mStrand.wrap( boost::bind(&ChatSession::readHandler,
                        shared_from_this(), _1)));
    } else {
        LOG(WARNING) << "readHandler - Error! Client will leave the room";
        mRoom.leaveRoom(shared_from_this());
    }
}

void ChatSession::writeHandler(const boost::system::error_code &error)
{
    if (!error) {
        mWriteMsgs.pop_front();
        if (!mWriteMsgs.empty()) {
            boost::asio::async_write(mSocket, boost::asio::buffer(
                            mWriteMsgs.front(), mWriteMsgs.front().size()),
                    mStrand.wrap(boost::bind(&ChatSession::writeHandler,
                            shared_from_this(), _1)));
        }
    } else {
        LOG(WARNING) << "writeHandler - Error! Client will leave the room";
        mRoom.leaveRoom(shared_from_this());
    }
}

void ChatSession::nicknameHandler(const boost::system::error_code &error)
{
    strcat(mNickname.data(), ": ");
    LOG(INFO) << mNickname.data() << " joined the chat";
    mRoom.joinRoom(shared_from_this(), std::string(mNickname.data()));

    boost::asio::async_read(mSocket,
            boost::asio::buffer(mReadMsg, mReadMsg.size()),
            mStrand.wrap(boost::bind(&ChatSession::readHandler,
                shared_from_this(), _1)));
}

//----------------------------------------------------------------------

ChatServer::ChatServer(boost::asio::io_context &io_context,
            boost::asio::io_context::strand &strand,
            const tcp::endpoint &endpoint)
    : mIo_context(io_context), mStrand(strand),
      mAcceptor(io_context, endpoint)
{
    startAccept();
}

void ChatServer::startAccept()
{
    chatSessionPtr new_session(new ChatSession(mIo_context, mStrand, mRoom));
    mAcceptor.async_accept(
            new_session->getSocket(),
            mStrand.wrap(boost::bind(&ChatServer::handleAccept, this,
                    new_session, boost::asio::placeholders::error)));
}

void ChatServer::handleAccept(chatSessionPtr session, const boost::system::error_code &error)
{
    LOG(INFO) << "Client connecting";
    if (!error)
        session->start();
    startAccept();
}

//----------------------------------------------------------------------

// Functionality to exit server even a bit more gracefully
void keepAliveThreadFunction()
{
    std::string input;
    while (1) {
        std::getline(std::cin, input);
        if (input == "exit") {
            exit(0);
        }
    }
}

//-----------------------------------------------------------------------

int main(int argc, char *argv[])
{
    auto worker = g3::LogWorker::createLogWorker();
    auto defaultHandler = worker->addDefaultLogger(argv[0], "./log/");
    g3::initializeLogging(worker.get());

    LOG(INFO) << "Starting chat server";
    try {
        if (argc < 2)
            LOG(FATAL) << "Wrong usage: ChatBoxServer <port>";

        /* Open backup messages text file and write them to vector */
        /* Backup messages contains 100 last lines, aka max recent msgs*/
        if (argc == 3 ) {
            std::string str;
            in.open(argv[2]);
            if (in) {
                while (std::getline(in, str))
                    if(str.size() > 0)
                        backup_msg.push_back(str);
                }
            flag=1;
        }

        std::shared_ptr<boost::asio::io_context> io_context(
                new boost::asio::io_context);

        boost::shared_ptr<boost::asio::io_context::work> work(
                new boost::asio::io_context::work(*io_context));

        boost::shared_ptr<boost::asio::io_context::strand> strand(
                new boost::asio::io_context::strand(*io_context));

        tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[1]));
        std::shared_ptr<ChatServer> a_server(
                new ChatServer(*io_context, *strand, endpoint));

        boost::thread *th =
                new boost::thread(boost::bind(&WorkerThread::run, io_context));

        std::thread alive_thread(keepAliveThreadFunction);
        alive_thread.detach();

        th->join();

    }
    catch (std::exception &e)
    {
            LOG(FATAL) << "Exception: " << e.what();
    }
    return 0;
}
