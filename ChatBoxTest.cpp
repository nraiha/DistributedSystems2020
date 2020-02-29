//
// ChatBox.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/thread/thread.hpp>
#include "ChatBox.h"
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <ctime>

using boost::asio::ip::tcp;
int vittuflag=0;


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
    vittuflag=1;
    sleep(3);
    boost::asio::async_connect(mSocket,
		       eIterator,
		       boost::bind(&ChatBox::handleConnect, this, ec));

}

void ChatBox::handleRead(const boost::system::error_code &error)
{
    if (vittuflag==0)
    	std::cout << mReadMsgs.data() << std::endl;

    vittuflag=0;

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


	    std::stringstream ss;

	/*
	 * Max packet size == 512
	 */
	int i;
        customArray msg;

	// min size packet
	std::string min = "";
	// avg size packet
	std::string avg = "abcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqr";
	// Max size packet
	std::string max = "abcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqrabcdefghijklmnopqr";

	int j;

	memset(msg.data(), '\0', msg.size());
	char ch[min.size()+1];
	strcpy(ch, &min[0]);
	strcpy(msg.data(), ch);

	clock_t total=0;
for (j=0; j<10000; j++)
{
	const clock_t begin1 = clock();
	for (i=0; i<50; i++) {
		cli.write(msg);
	}
	const clock_t end1 = clock();
	const clock_t duration1 = float(end1 - begin1);
	total += duration1;
}
	std::cout << "Send 25 min packets, average time taken: " << total << "\n";


	memset(msg.data(), '\0', msg.size());
	char ch2[avg.size()+1];
	strcpy(ch2, &avg[0]);
	strcpy(msg.data(), ch2);

total=0;
for (j=0; j<10000; j++)
{
	const clock_t begin2 = clock();
	for (i=0; i<50; i++) {
		cli.write(msg);
	}
	const clock_t end2 = clock();
	const clock_t duration2 = float(end2 - begin2);
	total += duration2;
}
	std::cout << "Send 25 avg packets, time taken: " << total << "\n";


	memset(msg.data(), '\0', msg.size());
	char ch3[max.size()+1];
	strcpy(ch3, &max[0]);
	strcpy(msg.data(), ch3);

total=0;
for (j=0; j<10000; j++)
{
	const clock_t begin3 = clock();
	for (i=0; i<50; i++) {
		cli.write(msg);
	}
	const clock_t end3 = clock();
	const clock_t duration3 = float(end3 - begin3) ;
	total += duration3;
}

	std::cout << "Send 25 max packets, time taken: " << total << "\n";

	std::cout << "\n\n" << CLOCKS_PER_SEC << std::endl;

#if 0
    std::string str = "joined the chat";
    char ch[str.size() + 1];
    strcpy(ch, &str[0]);
    // Copy the message from chat to customArray and broadcast it
    customArray message;
    strcpy(message.data(), ch);
#endif
#if 0
        while (true)
        {
            memset(msg.data(), '\0', msg.size());
            if(!std::cin.getline(msg.data(), MAX_PACK_SIZE - PADDING - MAX_NICKNAME_SIZE))
            {
                std::cin.clear();
            }
		ss << msg.data();
		if (ss.str() == "/exit") {
			std::cout << "Exiting..." << std::endl;
			exit(0);
		}

            cli.write(msg);

        }
#endif
            cli.close();
            th.join();

    } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
