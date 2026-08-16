#include <iostream>
#include <map>
#include <stack>
#include <vector>

#define ONLINE_JUDGE

std::string ShuntingYard(std::string operation)
{
	std::string postfix;
	std::stack<char> stack;
	std::map<char, int> priority = {
		{ '+', 1 },
		{ '-', 1 },
		{ '*', 2 },
		{ '/', 2 },
	};
	for (char c : operation) {
		if (c >= 'A' && c <= 'Z') {
			postfix += c;
		} else if (c == '+' || c == '-' || c == '*' || c == '/') {
			while (!stack.empty() &&
			       priority[stack.top()] >= priority[c] &&
			       stack.top() != '(') {
				postfix += stack.top();
				stack.pop();
			}
			stack.push(c);
		} else if (c == '(') {
			stack.push(c);
		} else if (c == ')') {
			while (!stack.empty() && stack.top() != '(') {
				postfix += stack.top();
				stack.pop();
			}
			stack.pop();
		}
	}
	while (!stack.empty()) {
		postfix += stack.top();
		stack.pop();
	}
	return postfix;
}

int main(void)
{
#ifndef ONLINE_JUDGE
	freopen("testcases.txt", "r", stdin);
	freopen("output.txt", "w", stdout);
#endif
	std::string operation;
	std::cin >> operation;
	std::string postfix = ShuntingYard(operation);
	int T;
	std::cin >> T;
	while (T--) {
		std::vector<int> values(5);
		for (int i = 0; i < 5; i++) {
			std::cin >> values[i];
		}
		std::stack<int> st;
		for (char c : postfix) {
			if (c >= 'A' && c <= 'Z') {
				st.push(values[c - 'A']);
			} else {
				int x = st.top();
				st.pop();
				int y = st.top();
				st.pop();
				if (c == '+') {
					st.push(y + x);
				} else if (c == '-') {
					st.push(y - x);
				} else if (c == '*') {
					st.push(y * x);
				} else if (c == '/') {
					st.push(y / x);
				}
			}
		}
		std::cout << st.top();
		if (T)
			std::cout << std::endl;
	}
	return 0;
}
