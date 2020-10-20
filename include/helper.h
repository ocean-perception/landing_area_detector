/**
 * @file helper.h
 * @author Jose Cappelletto (cappelletto@gmail.com)
 * @brief Collection of general helper functions
 * @version 0.2
 * @date 2020-07-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef _PROJECT_HELPER_H_
#define _PROJECT_HELPER_H_

#include <mutex>

using namespace std;

std::string type2str(int type);
std::string makeFixedLength(const int i, const int length);

/**
 * @brief 
 * 
 */
enum MessageTypes{
    MSG_INFO    = 1,    // Standard informative message
    MSG_WARNING = 2,    // non-critical warning message
    MSG_ERROR   = 3     // critical error message
};

class ConsoleOutput{
    private:
        std::vector<std::string> logHistory; // history of all received messages
        int counter; //number of calls to publish. It should match logHistory size
        std::mutex mtx;
    protected:
    public:
        ConsoleOutput(){
            counter = 0; //
        };
        ~ConsoleOutput(){
            this->logHistory.clear();
        };

        int publish(MessageTypes type, std::string owner, std::string message);
        void clear(); // clear the history log
        int  size();  // return the number of log entries
        void dump();  // dump (on screen or file) the log
};


#endif // _PROJECT_HELPER_H_