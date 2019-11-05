/*
 * util.h
 * This header file has all the utility functions used by whole project
 *
 *  Created on: Sep 20, 2019
 *      Author: Swapnil Bhosale
 */

#ifndef SERVER_UTIL_H_
#define SERVER_UTIL_H_
#include <iostream>
#include <string>
#include <sstream>
#include <iterator>

template <size_t N>
void splitString(std::string (&arr)[N], std::string str)
{
	int n = 0;
	std::istringstream iss(str);
	for (auto it = std::istream_iterator<std::string>(iss); it != std::istream_iterator<std::string>() && n < N; ++it, ++n)
		arr[n] = *it;
}

#endif /* SERVER_UTIL_H_ */
