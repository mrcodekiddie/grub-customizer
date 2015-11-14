/*
 * Copyright (C) 2010-2014 Daniel Richter <danielrichter2007@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef SRC_LIB_TESTPROCESS_HPP_
#define SRC_LIB_TESTPROCESS_HPP_

#include "ThreadQueue.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <chrono>

int main()
{
	ThreadQueue q(10);

	int i = 0;
	q.onReceive = [&] {
		if (i % 25 == 0) {
			std::cout << std::endl << i << " ";
		}
		std::cout << q.pop();
		std::cout.flush();
		i++;
	};

	std::thread t(
		[&q] {
			for (int i = 0; i < 1000; i++) {
				q.push('.');
			}
		}
	);
	std::thread t2(
		[&q] {
			for (int i = 0; i < 10000; i++) {
				q.push('*');
			}
		}
	);

//	for (int i = 0; i < 2000; i++) {
//		if (i % 25 == 0) {
//			std::cout << std::endl << i << " ";
//		}
//		std::cout << q.pop();
//		std::cout.flush();
//	}

	t.join();
	t2.join();
}

#endif /* SRC_LIB_TESTPROCESS_HPP_ */
