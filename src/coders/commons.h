#ifndef CODERS_COMMONS_H_
#define CODERS_COMMONS_H_

#include <string>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <stdexcept>

class BasicParser {
protected:
	std::string filename;
	std::string source;
	size_t pos;
	size_t line;
	size_t linestart;

	BasicParser(std::string filename, std::string source);
	
	char peek();
	char nextChar();
	bool hasNext();
	void expect(char c);
	void expectNewLine();
	virtual void skipWhitespace();
	
	std::string parseName();
	double parseNumber(int sign);
	std::string parseString(char quote);
	
	std::runtime_error error(std::string message);
};

inline bool is_whitespace(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

inline bool is_identifier_start(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool is_identifier_char(char c) {
	return is_identifier_start(c) || is_digit(c);
}

std::string escape_string(const std::string& str);

#endif /* CODERS_COMMONS_H_ */

