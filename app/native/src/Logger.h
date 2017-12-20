/*
Logger.h - Logs information about the compositor to a file
author: Evan Shimizu
*/

#pragma once
#ifndef _COMP_LOGGER_H_
#define _COMP_LOGGER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <time.h>
#include <iomanip>

using namespace std;

namespace Comp
{
  enum LogLevel {
    SILLY = -1,
    DBG = 0,
    INFO = 1,
    WARN = 2,
    ERR = 3,
    FATAL = 4
  };

  class Logger
  {
  public:
    Logger();
    ~Logger();

    void setLogLocation(string loc);
    void log(string msg, LogLevel level = LogLevel::DBG);
    void setLogLevel(LogLevel level);

  private:
    string printTime();
    string logLevelToString(LogLevel level);

    ofstream _file;
    int _level;
  };

  // program-wide access to logger. Access should be through getLogger,
  // which will initialize the logger if it doesn't exist.
  static shared_ptr<Logger> _logger = nullptr;
  static shared_ptr<Logger> getLogger()
  {
    if (_logger == nullptr) {
      _logger = shared_ptr<Logger>(new Logger());
    }

    return _logger;
  }
}

#endif