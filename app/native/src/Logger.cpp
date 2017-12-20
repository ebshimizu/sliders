#include "Logger.h"

namespace Comp {
  Logger::Logger()
  {
    setLogLocation("./");
    setLogLevel(DBG);
  }

  Logger::~Logger()
  {
    _file.close();
  }

  void Logger::setLogLocation(string loc)
  {
    _file.open(loc + "compositor.log", ios::out | ios::app);
  }

  void Logger::log(string msg, LogLevel level)
  {
    if ((int)level >= _level) {
      if (_file.is_open()) {
        _file << "[" << logLevelToString(level) << "]\t" << printTime() << " " << msg << "\n";
        _file.flush();
      }
    }
  }

  void Logger::setLogLevel(LogLevel level)
  {
    _level = (int)level;
  }

  string Logger::printTime()
  {
    // C++11 chrono used for timestamp
    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    stringstream buf;

#ifndef __linux__
    buf << put_time(localtime(&now), "%Y-%m-%d %H:%M:%S");
#else
    char buffer[80];
    strftime(buffer, 80, "%d-%m-%Y %I:%M:%S", localtime(&now));
    std::string timebuf(buffer);
    buf << timebuf;
#endif

    return buf.str();
  }

  string Logger::logLevelToString(LogLevel level)
  {
    switch (level) {
    case(SILLY): return "SILLY";
    case(DBG):   return "DEBUG";
    case(INFO):  return "INFO";
    case(WARN):  return "WARN";
    case(ERR):   return "ERROR";
    case(FATAL): return "FATAL";
    default:     return "";
    }
  } 
}
