
/*
 *
 * ChatBox.cpp
 *
 */

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/thread/thread.hpp>

#include "ChatBox.h"

using boost::asio::ip::tcp;
int reconnectFlag=0;

//-----------------------------------------------------------------------

ChatBox::ChatBox(std::array<char, MAX_NICKNAME_SIZE>& clientName,
                         boost::asio::io_context& io_context,
                         tcp::resolver::iterator endpoint_iterator)
        : mIo_context(io_context), mSocket(io_context), eIterator(endpoint_iterator)
{
    strcpy(mClientName.data(), clientName.data());
    memset(mReadMsgs.data(), '\0', MAX_PACK_SIZE);
    boost::asio::async_connect(mSocket,
                               eIterator,
                               boost::bind(&ChatBox::handleConnect, this, _1));
}

void ChatBox::write(const customArray& msg)
{
        mIo_context.post(boost::bind(&ChatBox::doWrite, this, msg));
}

void ChatBox::close()
{
        mIo_context.post(boost::bind(&ChatBox::doClose, this));
}


void ChatBox::handleConnect(const boost::system::error_code& error)
{
        if (!error) {
            boost::asio::async_write(mSocket,
                                     boost::asio::buffer(mClientName, mClientName.size()),
                                     boost::bind(&ChatBox::handleRead, this, _1));
        }
}

void ChatBox::doWrite(customArray msg)
{
        bool write_in_progress = !mWriteMsgs.empty();
        mWriteMsgs.push_back(msg);
        if (!write_in_progress) {
            boost::asio::async_write(mSocket,
                                     boost::asio::buffer(mWriteMsgs.front(), mWriteMsgs.front().size()),
                                     boost::bind(&ChatBox::handleWrite, this, _1));
        }
}

void ChatBox::handleWrite(const boost::system::error_code& error)
{
        if (!error) {
            mWriteMsgs.pop_front();
            if (!mWriteMsgs.empty()) {
                boost::asio::async_write(mSocket,
                                         boost::asio::buffer(
                                         mWriteMsgs.front().data(),
                                         mWriteMsgs.front().size()),
                        boost::bind(&ChatBox::handleWrite, this, boost::asio::placeholders::error));
            }
        } else {
            doClose();
        }
}

void ChatBox::reconnect()
{
    const boost::system::error_code ec;
    std::cout << "Trying to reconnect" << std::endl;
    reconnectFlag=1;
    sleep(3);
    boost::asio::async_connect(mSocket,
               eIterator,
               boost::bind(&ChatBox::handleConnect, this, ec));

}

void ChatBox::handleRead(const boost::system::error_code &error)
{
    if (reconnectFlag==0)
        std::cout << mReadMsgs.data() << std::endl;

    reconnectFlag=0;

    if(!error)
    {
        boost::asio::async_read(mSocket,
                            boost::asio::buffer(mReadMsgs, mReadMsgs.size()),
                                boost::bind(&ChatBox::handleRead, this, _1));
    }
    else
    {
        doClose();
        reconnect();
    }
}

void ChatBox::doClose()
{
        mSocket.close();
}

//-----------------------------------------------------------------------

int main(int argc, char* argv[])
{
    try {
        if (argc != 4) {
                std::cout << "Usage: ChatBox <name> <host> <port>" << std::endl;
                return 1;
        }

        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::query query(argv[2], argv[3]);
        tcp::resolver::iterator iterator = resolver.resolve(query);
        std::array<char, MAX_NICKNAME_SIZE> nickname;
        strcpy(nickname.data(), argv[1]);

        ChatBox cli(nickname, io_context, iterator);

        std::thread th(boost::bind(&boost::asio::io_context::run, &io_context));

        customArray msg;
        std::stringstream ss;
        while (true)
        {
	    ss.str(std::string());
            memset(msg.data(), '\0', 0);
            if(!std::cin.getline(msg.data(), 
            MAX_PACK_SIZE - PADDING - MAX_NICKNAME_SIZE)) {
                std::cin.clear();
            }
            ss << msg.data();
            if (ss.str() == "/exit") {
                std::cout << "Exiting..." << std::endl;
                exit(0);
            }

            cli.write(msg);
        }

            cli.close();
            th.join();

    } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
