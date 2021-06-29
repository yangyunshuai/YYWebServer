//
// Created by marvinle on 2019/2/1 12:07 PM.
//

#ifndef UTILS_H
#define UTILS_H


#include <string>

using namespace std;

std::string &ltrim(string &);

std::string &rtrim(string &);

std::string &trim(string &);

int setnonblocking(int fd);
// void handle_for_sigpipe();

int check_base_path(char *basePath);




#endif
